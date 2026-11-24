#include "db.h"
#include "coding.h"
#include "error_code.h"
#include "comparator.h"
#include "env.h"

#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace wedb {

DB::DB(void)
: opened_(false),
  lock_fd_(-1),
  last_sequence_(0),
  next_file_number_(1),
  log_number_(0)
{
}

DB::~DB(void)
{
    close();
}

bool DB::fail(int code, const std::string &message)
{
    set_err_code(code);
    last_error_ = message;
    return false;
}

std::string DB::manifest_path(void) const
{
    return path_join(root_path_, "MANIFEST");
}

std::string DB::lock_path(void) const
{
    return path_join(root_path_, "LOCK");
}

std::string DB::level_dir(int level) const
{
    return path_join(root_path_, std::string("L") + std::to_string(level));
}

std::string DB::table_path(int level, uint64_t number) const
{
    return path_join(level_dir(level), std::to_string(number) + ".sst");
}

std::string DB::wal_path(uint64_t number) const
{
    return path_join(root_path_, log_file_name(number));
}

void DB::close_readers_locked(void)
{
    readers_.clear();
}

void DB::release_lock_locked(void)
{
    if (lock_fd_ >= 0) {
        ::flock(lock_fd_, LOCK_UN);
        ::close(lock_fd_);
        lock_fd_ = -1;
    }
}

bool DB::open_reader_locked(const FileMetaData &file)
{
    std::map<uint64_t, std::unique_ptr<TableReader> >::iterator iter =
        readers_.find(file.number);
    if (iter != readers_.end()) {
        return true;
    }

    std::unique_ptr<TableReader> reader(new TableReader());
    if (!reader->open(table_path(file.level, file.number), file.number)) {
        return fail(ERROR_Code_Corruption,
                    std::string("open table failed: ") + table_path(file.level, file.number));
    }
    readers_[file.number] = std::move(reader);
    return true;
}

bool DB::open_readers_locked(void)
{
    for (int level = 0; level < version_.num_levels(); ++level) {
        const std::vector<FileMetaData> &files = version_.files(level);
        for (size_t i = 0; i < files.size(); ++i) {
            if (!open_reader_locked(files[i])) {
                return false;
            }
        }
    }
    return true;
}

bool DB::save_manifest_locked(void)
{
    if (!save_manifest_atomic(manifest_path(), version_, last_sequence_,
                              next_file_number_, log_number_)) {
        return fail(ERROR_Code_IO, "save manifest failed");
    }
    return true;
}

bool DB::create_wal_locked(uint64_t number, std::unique_ptr<WalWriter> *writer)
{
    if (writer == nullptr) {
        return false;
    }
    std::unique_ptr<WalWriter> wal(new WalWriter());
    if (!wal->open(wal_path(number), options_.sync)) {
        return fail(ERROR_Code_IO, std::string("open wal failed: ") + wal_path(number));
    }
    *writer = std::move(wal);
    return true;
}

bool DB::remove_logs_except_locked(uint64_t keep_number)
{
    std::vector<std::string> names;
    if (!list_dir(root_path_, &names)) {
        return false;
    }
    for (size_t i = 0; i < names.size(); ++i) {
        uint64_t number = 0;
        if (parse_log_file_name(names[i], &number) && number != keep_number) {
            remove_file(path_join(root_path_, names[i]));
        }
    }
    return true;
}

bool DB::cleanup_tmp_files_locked(void)
{
    std::vector<std::string> root_names;
    if (list_dir(root_path_, &root_names)) {
        for (size_t i = 0; i < root_names.size(); ++i) {
            if (root_names[i].size() >= 4 &&
                root_names[i].compare(root_names[i].size() - 4, 4, ".tmp") == 0) {
                remove_file(path_join(root_path_, root_names[i]));
            }
        }
    }

    for (int level = 0; level < version_.num_levels(); ++level) {
        std::vector<std::string> names;
        if (!list_dir(level_dir(level), &names)) {
            continue;
        }
        for (size_t i = 0; i < names.size(); ++i) {
            if (names[i].size() >= 4 &&
                names[i].compare(names[i].size() - 4, 4, ".tmp") == 0) {
                remove_file(path_join(level_dir(level), names[i]));
            }
        }
    }
    return true;
}

bool DB::close(void)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (wal_writer_.get() != nullptr) {
        wal_writer_->close();
        wal_writer_.reset();
    }
    close_readers_locked();
    release_lock_locked();
    opened_ = false;
    active_.clear();
    version_.clear();
    set_err_code(ERROR_OK);
    return true;
}

namespace {

bool parse_table_file_name(const std::string &name, uint64_t *number)
{
    const std::string suffix = ".sst";
    if (number == nullptr || name.size() <= suffix.size() ||
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) {
        return false;
    }

    const std::string digits = name.substr(0, name.size() - suffix.size());
    if (digits.empty()) {
        return false;
    }
    uint64_t value = 0;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (digits[i] < '0' || digits[i] > '9') {
            return false;
        }
        value = value * 10 + static_cast<uint64_t>(digits[i] - '0');
    }
    *number = value;
    return true;
}

} // namespace

bool DB::open(const std::string &path, const DBOptions &options)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (opened_) {
        return fail(ERROR_Code_AlreadyExists, "db already open");
    }

    if (path.empty()) {
        return fail(ERROR_Code_InvalidArgument, "empty db path");
    }

    options_ = options;
    if (options_.num_levels < 1) {
        options_.num_levels = 1;
    }
    if (options_.compaction_trigger < 2) {
        options_.compaction_trigger = 2;
    }
    if (options_.write_buffer_size == 0) {
        options_.write_buffer_size = 5 * 1024 * 1024;
    }
    if (options_.block_size == 0) {
        options_.block_size = 4096;
    }

    root_path_ = path;
    opened_ = false;
    last_error_.clear();
    last_sequence_ = 0;
    next_file_number_ = 1;
    log_number_ = 0;
    active_.clear();
    version_.clear();
    close_readers_locked();
    wal_writer_.reset();
    release_lock_locked();

    if (!mkdirs(root_path_)) {
        return fail(ERROR_Code_IO, std::string("mkdir failed: ") + root_path_);
    }
    for (int level = 0; level < options_.num_levels; ++level) {
        if (!mkdirs(level_dir(level))) {
            return fail(ERROR_Code_IO, std::string("mkdir failed: ") + level_dir(level));
        }
    }

    lock_fd_ = ::open(lock_path().c_str(), O_RDWR | O_CREAT, 0644);
    if (lock_fd_ < 0) {
        return fail(ERROR_Code_IO, std::string("open lock failed: ") + lock_path());
    }
    if (::flock(lock_fd_, LOCK_EX | LOCK_NB) != 0) {
        release_lock_locked();
        return fail(ERROR_Code_Locked, std::string("db is locked: ") + lock_path());
    }

    auto abort_open = [this](int code, const std::string &message) -> bool {
        wal_writer_.reset();
        close_readers_locked();
        release_lock_locked();
        version_.clear();
        active_.clear();
        opened_ = false;
        return fail(code, message);
    };

    if (file_exists(manifest_path())) {
        if (options_.error_if_exists) {
            return abort_open(ERROR_Code_AlreadyExists,
                              std::string("db already exists: ") + root_path_);
        }
        if (!load_manifest(manifest_path(), options_.num_levels, &version_,
                           &last_sequence_, &next_file_number_, &log_number_)) {
            return abort_open(ERROR_Code_Corruption,
                              std::string("invalid manifest: ") + manifest_path());
        }
    } else {
        if (!options_.create_if_missing) {
            return abort_open(ERROR_Code_NotFound,
                              std::string("db not found: ") + root_path_);
        }
        version_.init(options_.num_levels);
        last_sequence_ = 0;
        next_file_number_ = 1;
        log_number_ = 0;
    }

    if (!open_readers_locked()) {
        return abort_open(ERROR_Code_Corruption, last_error_.empty()
                              ? std::string("open tables failed")
                              : last_error_);
    }

    uint64_t max_number = 0;
    std::vector<std::string> names;
    if (list_dir(root_path_, &names)) {
        for (size_t i = 0; i < names.size(); ++i) {
            uint64_t number = 0;
            if (parse_log_file_name(names[i], &number) && number > max_number) {
                max_number = number;
            }
        }
    }
    for (int level = 0; level < options_.num_levels; ++level) {
        names.clear();
        if (!list_dir(level_dir(level), &names)) {
            continue;
        }
        for (size_t i = 0; i < names.size(); ++i) {
            uint64_t number = 0;
            if (parse_table_file_name(names[i], &number) && number > max_number) {
                max_number = number;
            }
        }
    }
    if (next_file_number_ == 0) {
        next_file_number_ = 1;
    }
    if (next_file_number_ <= max_number) {
        next_file_number_ = max_number + 1;
    }

    if (!replay_logs_locked()) {
        return abort_open(ERROR_Code_Corruption, last_error_.empty()
                              ? std::string("replay wal failed")
                              : last_error_);
    }

    cleanup_tmp_files_locked();
    opened_ = true;
    set_err_code(ERROR_OK);
    return true;
}

bool DB::replay_logs_locked(void)
{
    std::vector<std::string> names;
    if (!list_dir(root_path_, &names)) {
        return fail(ERROR_Code_IO, std::string("list dir failed: ") + root_path_);
    }

    std::vector<uint64_t> logs;
    for (size_t i = 0; i < names.size(); ++i) {
        uint64_t number = 0;
        if (parse_log_file_name(names[i], &number) && number >= log_number_) {
            logs.push_back(number);
        }
    }
    std::sort(logs.begin(), logs.end());

    for (size_t i = 0; i < logs.size(); ++i) {
        std::vector<Entry> entries;
        bool truncated = false;
        if (!read_wal(wal_path(logs[i]), &entries, &truncated)) {
            return fail(ERROR_Code_Corruption,
                        std::string("read wal failed: ") + wal_path(logs[i]));
        }
        (void)truncated;
        for (size_t j = 0; j < entries.size(); ++j) {
            active_.add(entries[j]);
            if (entries[j].seq() > last_sequence_) {
                last_sequence_ = entries[j].seq();
            }
        }
    }

    if (!active_.empty()) {
        return flush_locked();
    }

    uint64_t new_log = next_file_number_++;
    std::unique_ptr<WalWriter> wal;
    if (!create_wal_locked(new_log, &wal)) {
        return false;
    }
    wal_writer_ = std::move(wal);
    log_number_ = new_log;
    if (!save_manifest_locked()) {
        return false;
    }
    remove_logs_except_locked(new_log);
    return true;
}

bool DB::put(const std::string &key, const std::string &value, bool sync)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!opened_) {
        return fail(ERROR_Code_Internal, "db not open");
    }
    if (key.empty() || key.size() > 0xFFFFU || value.size() > 0xFFFFFFU) {
        return fail(ERROR_Code_InvalidArgument, "invalid key or value size");
    }

    Entry entry(++last_sequence_, EntryType_Put, key, value);
    if (wal_writer_.get() == nullptr ||
        !wal_writer_->append(entry, sync || options_.sync)) {
        return fail(ERROR_Code_IO, "append wal failed");
    }
    if (!active_.add(entry)) {
        return fail(ERROR_Code_Internal, "add memtable failed");
    }

    if (active_.approximate_memory_usage() >= options_.write_buffer_size) {
        if (!flush_locked()) {
            return false;
        }
    }

    set_err_code(ERROR_OK);
    return true;
}

bool DB::put_buffer(const std::string &key, const basic::ByteBuffer &value, bool sync)
{
    basic::ByteBuffer copy(value);
    return put(key, copy.str(), sync);
}

bool DB::remove(const std::string &key, bool sync)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!opened_) {
        return fail(ERROR_Code_Internal, "db not open");
    }
    if (key.empty() || key.size() > 0xFFFFU) {
        return fail(ERROR_Code_InvalidArgument, "invalid key size");
    }

    Entry entry(++last_sequence_, EntryType_Delete, key, std::string());
    if (wal_writer_.get() == nullptr ||
        !wal_writer_->append(entry, sync || options_.sync)) {
        return fail(ERROR_Code_IO, "append wal failed");
    }
    if (!active_.add(entry)) {
        return fail(ERROR_Code_Internal, "add memtable failed");
    }

    if (active_.approximate_memory_usage() >= options_.write_buffer_size) {
        if (!flush_locked()) {
            return false;
        }
    }

    set_err_code(ERROR_OK);
    return true;
}

bool DB::get(const std::string &key, std::string *value)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!opened_) {
        return fail(ERROR_Code_Internal, "db not open");
    }
    if (key.empty() || value == nullptr) {
        return fail(ERROR_Code_InvalidArgument, "invalid key or output");
    }

    Entry best;
    bool found = false;

    Entry temp;
    if (active_.get(key, &temp)) {
        best = temp;
        found = true;
    }

    for (int level = 0; level < version_.num_levels(); ++level) {
        const std::vector<FileMetaData> &files = version_.files(level);
        for (size_t i = 0; i < files.size(); ++i) {
            std::map<uint64_t, std::unique_ptr<TableReader> >::iterator iter =
                readers_.find(files[i].number);
            if (iter == readers_.end() || iter->second.get() == nullptr) {
                continue;
            }
            const bool hit = iter->second->get(key, &temp);
            if (!hit && iter->second->corrupted()) {
                return fail(ERROR_Code_Corruption, "table block corrupted");
            }
            if (hit) {
                if (!found || temp.seq() > best.seq()) {
                    best = temp;
                    found = true;
                }
            }
        }
    }

    if (!found || best.is_delete()) {
        return fail(ERROR_Code_NotFound, "key not found");
    }

    *value = best.value();
    set_err_code(ERROR_OK);
    return true;
}

bool DB::get_buffer(const std::string &key, basic::ByteBuffer *value)
{
    if (value == nullptr) {
        return fail(ERROR_Code_InvalidArgument, "null output buffer");
    }
    std::string result;
    if (!get(key, &result)) {
        return false;
    }
    *value = basic::ByteBuffer(result);
    return true;
}

bool DB::scan(const std::string &start_key, const std::string &end_key,
              const ScanCallback &callback)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!opened_) {
        return fail(ERROR_Code_Internal, "db not open");
    }
    if (!callback) {
        return fail(ERROR_Code_InvalidArgument, "empty scan callback");
    }

    std::vector<std::unique_ptr<Iterator> > children;
    children.push_back(active_.new_iterator());
    for (std::map<uint64_t, std::unique_ptr<TableReader> >::iterator iter = readers_.begin();
         iter != readers_.end(); ++iter) {
        children.push_back(iter->second->new_iterator());
    }

    MergeIterator merge(std::move(children));
    merge.seek(start_key);

    BytewiseComparator compare;
    while (merge.valid()) {
        const Entry &entry = merge.entry();
        if (!end_key.empty() && compare.compare(entry.key(), end_key) >= 0) {
            break;
        }
        if (!entry.is_delete()) {
            if (!callback(entry.key(), entry.value())) {
                break;
            }
        }
        merge.next();
    }

    set_err_code(ERROR_OK);
    return true;
}

bool DB::flush(void)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!opened_) {
        return fail(ERROR_Code_Internal, "db not open");
    }
    return flush_locked();
}

bool DB::compact(void)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (!opened_) {
        return fail(ERROR_Code_Internal, "db not open");
    }
    return compact_locked();
}

bool DB::write_active_table_locked(FileMetaData *meta)
{
    if (meta == nullptr || active_.empty()) {
        return false;
    }

    uint64_t number = next_file_number_++;
    TableBuilderOptions options;
    options.level = 0;
    options.file_number = number;
    options.block_size = options_.block_size;
    options.bits_per_key = options_.bits_per_key;

    TableBuilder builder;
    const std::string path = table_path(0, number);
    if (!builder.open(path, options)) {
        return fail(ERROR_Code_IO, std::string("open table builder failed: ") + path);
    }

    std::unique_ptr<Iterator> iter = active_.new_iterator();
    iter->seek("");
    while (iter->valid()) {
        if (!builder.add(iter->entry())) {
            return fail(ERROR_Code_IO, std::string("write table failed: ") + path);
        }
        iter->next();
    }

    if (!builder.finish(meta)) {
        remove_file(path);
        return fail(ERROR_Code_IO, std::string("finish table failed: ") + path);
    }
    return true;
}

bool DB::flush_locked(void)
{
    if (active_.empty()) {
        return true;
    }
    const uint64_t new_log = next_file_number_++;

    std::unique_ptr<WalWriter> new_wal;
    if (!create_wal_locked(new_log, &new_wal)) {
        return false;
    }

    FileMetaData meta;
    if (!write_active_table_locked(&meta)) {
        new_wal->close();
        remove_file(wal_path(new_log));
        return false;
    }

    Version new_version = version_;
    if (!new_version.add_file(meta)) {
        remove_file(table_path(0, meta.number));
        new_wal->close();
        remove_file(wal_path(new_log));
        return fail(ERROR_Code_Internal, "add flushed file to version failed");
    }

    if (!save_manifest_atomic(manifest_path(), new_version, last_sequence_,
                              next_file_number_, new_log)) {
        remove_file(table_path(0, meta.number));
        new_wal->close();
        remove_file(wal_path(new_log));
        return fail(ERROR_Code_IO, "save manifest after flush failed");
    }

    version_ = new_version;
    if (wal_writer_.get() != nullptr) {
        wal_writer_->close();
    }
    wal_writer_ = std::move(new_wal);
    log_number_ = new_log;
    active_.clear();

    if (!open_reader_locked(meta)) {
        return false;
    }
    remove_logs_except_locked(new_log);

    if (!maybe_compact_locked()) {
        return false;
    }

    set_err_code(ERROR_OK);
    return true;
}

bool DB::write_merged_table_locked(std::vector<std::unique_ptr<Iterator> > *children,
                                   int level, uint64_t number, bool drop_tombstones,
                                   FileMetaData *meta)
{
    if (children == nullptr || meta == nullptr) {
        return false;
    }

    TableBuilderOptions options;
    options.level = level;
    options.file_number = number;
    options.block_size = options_.block_size;
    options.bits_per_key = options_.bits_per_key;

    const std::string path = table_path(level, number);
    TableBuilder builder;
    if (!builder.open(path, options)) {
        return fail(ERROR_Code_IO, std::string("open compaction table failed: ") + path);
    }

    MergeIterator merge(std::move(*children));
    bool added = false;
    while (merge.valid()) {
        const Entry &entry = merge.entry();
        if (!(drop_tombstones && entry.is_delete())) {
            if (!builder.add(entry)) {
                return fail(ERROR_Code_IO, std::string("write compaction table failed: ") + path);
            }
            added = true;
        }
        merge.next();
    }

    if (!added) {
        return true;
    }

    if (!builder.finish(meta)) {
        remove_file(path);
        return fail(ERROR_Code_IO, std::string("finish compaction table failed: ") + path);
    }
    return true;
}

bool DB::maybe_compact_locked(void)
{
    return compact_locked();
}

bool DB::compact_locked(void)
{
    while (true) {
        int level = -1;
        for (int i = 0; i < version_.num_levels(); ++i) {
            if (version_.files(i).size() >= options_.compaction_trigger) {
                level = i;
                break;
            }
        }
        if (level < 0) {
            break;
        }

        const std::vector<FileMetaData> inputs = version_.files(level);
        const int target_level = std::min(level + 1, version_.num_levels() - 1);
        std::vector<std::unique_ptr<Iterator> > children;
        for (size_t i = 0; i < inputs.size(); ++i) {
            std::map<uint64_t, std::unique_ptr<TableReader> >::iterator iter =
                readers_.find(inputs[i].number);
            if (iter == readers_.end() || iter->second.get() == nullptr) {
                return fail(ERROR_Code_Corruption, "compaction input reader missing");
            }
            children.push_back(iter->second->new_iterator());
        }

        const uint64_t number = next_file_number_++;
        FileMetaData meta;
        const bool drop_tombstones = (level == version_.num_levels() - 1);
        if (!write_merged_table_locked(&children, target_level, number,
                                       drop_tombstones, &meta)) {
            return false;
        }

        Version new_version = version_;
        for (size_t i = 0; i < inputs.size(); ++i) {
            new_version.remove_file(level, inputs[i].number);
        }
        if (meta.number != 0) {
            meta.level = target_level;
            if (!new_version.add_file(meta)) {
                remove_file(table_path(target_level, meta.number));
                return fail(ERROR_Code_Internal, "add compaction file failed");
            }
        }

        if (!save_manifest_atomic(manifest_path(), new_version, last_sequence_,
                                  next_file_number_, log_number_)) {
            if (meta.number != 0) {
                remove_file(table_path(target_level, meta.number));
            }
            return fail(ERROR_Code_IO, "save manifest after compaction failed");
        }

        version_ = new_version;
        for (size_t i = 0; i < inputs.size(); ++i) {
            readers_.erase(inputs[i].number);
            remove_file(table_path(level, inputs[i].number));
        }
        if (meta.number != 0 && !open_reader_locked(meta)) {
            return false;
        }
    }

    set_err_code(ERROR_OK);
    return true;
}

} // namespace wedb

