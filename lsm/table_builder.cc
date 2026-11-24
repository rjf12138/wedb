#include "table_builder.h"
#include "coding.h"
#include "comparator.h"

namespace wedb {

namespace {
const char kTableMagic[9] = "WEDB0001";
const uint32_t kTableFormatVersion = 1;
const size_t kFooterSize = 48;
}

TableBuilder::TableBuilder(void)
: max_seq_(0),
  offset_(0),
  started_(false),
  finished_(false)
{
}

TableBuilder::~TableBuilder(void)
{
    if (started_ && !finished_) {
        cleanup();
    }
}

bool TableBuilder::open(const std::string &final_path, const TableBuilderOptions &options)
{
    if (started_) {
        return false;
    }
    if (!mkdirs(dir_name(final_path))) {
        return false;
    }

    options_ = options;
    final_path_ = final_path;
    tmp_path_ = final_path + ".tmp";
    if (!file_.open(tmp_path_, true)) {
        return false;
    }
    started_ = true;
    finished_ = false;
    offset_ = 0;
    max_seq_ = 0;
    index_.clear();
    block_.clear();
    bloom_ = BloomFilter(options_.bits_per_key);
    last_key_.clear();
    smallest_key_.clear();
    largest_key_.clear();
    return true;
}

bool TableBuilder::add(const Entry &entry)
{
    if (!started_ || finished_) {
        return false;
    }
    if (entry.key().empty()) {
        return false;
    }

    if (!block_.empty() && block_.size() >= options_.block_size) {
        if (!flush_block()) {
            return false;
        }
    }

    if (!block_.add(entry)) {
        return false;
    }
    BytewiseComparator compare;
    bloom_.add(entry.key());
    last_key_ = entry.key();
    if (smallest_key_.empty() || compare.compare(entry.key(), smallest_key_) < 0) {
        smallest_key_ = entry.key();
    }
    if (largest_key_.empty() || compare.compare(entry.key(), largest_key_) > 0) {
        largest_key_ = entry.key();
    }
    if (entry.seq() > max_seq_) {
        max_seq_ = entry.seq();
    }
    return true;
}

bool TableBuilder::flush_block(void)
{
    if (block_.empty()) {
        return true;
    }

    std::string data;
    if (!block_.finish(&data)) {
        return false;
    }

    IndexEntry index;
    index.last_key = last_key_;
    index.offset = offset_;
    index.size = static_cast<uint32_t>(data.size());
    index_.push_back(index);

    if (!file_.append(data, false)) {
        return false;
    }
    offset_ += data.size();
    block_.clear();
    return true;
}

bool TableBuilder::finish(FileMetaData *meta)
{
    if (!started_ || finished_ || meta == nullptr) {
        return false;
    }
    if (!flush_block()) {
        cleanup();
        return false;
    }
    if (index_.empty()) {
        cleanup();
        return false;
    }

    bloom_.build();
    std::string filter_block;
    if (!bloom_.encode(&filter_block)) {
        cleanup();
        return false;
    }
    uint64_t filter_offset = offset_;
    if (!file_.append(filter_block, false)) {
        cleanup();
        return false;
    }
    offset_ += filter_block.size();

    std::string index_block;
    put_fixed32(&index_block, static_cast<uint32_t>(index_.size()));
    for (size_t i = 0; i < index_.size(); ++i) {
        const IndexEntry &index = index_[i];
        if (index.last_key.size() > 0xFFFFU) {
            cleanup();
            return false;
        }
        put_fixed16(&index_block, static_cast<uint16_t>(index.last_key.size()));
        index_block.append(index.last_key);
        put_fixed64(&index_block, index.offset);
        put_fixed32(&index_block, index.size);
    }
    put_fixed32(&index_block, crc32(index_block.data(), index_block.size()));

    uint64_t index_offset = offset_;
    if (!file_.append(index_block, false)) {
        cleanup();
        return false;
    }
    offset_ += index_block.size();

    std::string footer;
    put_fixed64(&footer, filter_offset);
    put_fixed64(&footer, filter_block.size());
    put_fixed64(&footer, index_offset);
    put_fixed64(&footer, index_block.size());
    footer.append(kTableMagic, 8);
    put_fixed32(&footer, kTableFormatVersion);
    put_fixed32(&footer, crc32(footer.data(), footer.size()));
    if (footer.size() != kFooterSize) {
        cleanup();
        return false;
    }
    if (!file_.append(footer, false)) {
        cleanup();
        return false;
    }
    offset_ += footer.size();

    if (!file_.sync() || !file_.close()) {
        cleanup();
        return false;
    }
    if (!rename_file(tmp_path_, final_path_)) {
        cleanup();
        return false;
    }
    if (!sync_dir(dir_name(final_path_))) {
        cleanup();
        return false;
    }

    meta->level = options_.level;
    meta->number = options_.file_number;
    meta->file_size = offset_;
    meta->smallest_key = smallest_key_;
    meta->largest_key = largest_key_;
    meta->max_seq = max_seq_;

    finished_ = true;
    return true;
}

void TableBuilder::cleanup(void)
{
    file_.close();
    if (!tmp_path_.empty()) {
        remove_file(tmp_path_);
    }
    block_.clear();
    index_.clear();
    started_ = false;
}

} // namespace wedb
