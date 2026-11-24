#ifndef __WEDB_DB_H__
#define __WEDB_DB_H__

#include "manifest.h"
#include "memtable.h"
#include "table_builder.h"
#include "table_reader.h"
#include "version.h"
#include "wal.h"

#include "basic/byte_buffer.h"
#include "basic/err_handle.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace wedb {

struct DBOptions {
    bool create_if_missing;
    bool error_if_exists;
    bool sync;
    size_t write_buffer_size;
    size_t block_size;
    uint32_t bits_per_key;
    size_t compaction_trigger;
    int num_levels;

    DBOptions(void)
    : create_if_missing(true),
      error_if_exists(false),
      sync(false),
      write_buffer_size(5 * 1024 * 1024),
      block_size(4096),
      bits_per_key(10),
      compaction_trigger(4),
      num_levels(6) {}
};

typedef std::function<bool(const std::string &key, const std::string &value)> ScanCallback;

class DB : public basic::ErrHandle {
public:
    DB(void);
    virtual ~DB(void);

    bool open(const std::string &path, const DBOptions &options = DBOptions());
    bool close(void);
    bool is_open(void) const { return opened_; }

    bool put(const std::string &key, const std::string &value, bool sync = false);
    bool put_buffer(const std::string &key, const basic::ByteBuffer &value, bool sync = false);
    bool get(const std::string &key, std::string *value);
    bool get_buffer(const std::string &key, basic::ByteBuffer *value);
    bool remove(const std::string &key, bool sync = false);
    bool scan(const std::string &start_key, const std::string &end_key,
              const ScanCallback &callback);

    bool flush(void);
    bool compact(void);

    const std::string &last_error_message(void) const { return last_error_; }

private:
    DB(const DB &);
    DB &operator=(const DB &);

    bool fail(int code, const std::string &message);

    std::string manifest_path(void) const;
    std::string lock_path(void) const;
    std::string level_dir(int level) const;
    std::string table_path(int level, uint64_t number) const;
    std::string wal_path(uint64_t number) const;

    bool open_readers_locked(void);
    bool open_reader_locked(const FileMetaData &file);
    bool save_manifest_locked(void);
    bool create_wal_locked(uint64_t number, std::unique_ptr<WalWriter> *writer);
    bool remove_logs_except_locked(uint64_t keep_number);
    bool cleanup_tmp_files_locked(void);
    void close_readers_locked(void);
    void release_lock_locked(void);

    bool replay_logs_locked(void);
    bool flush_locked(void);
    bool compact_locked(void);
    bool maybe_compact_locked(void);
    bool write_active_table_locked(FileMetaData *meta);
    bool write_merged_table_locked(std::vector<std::unique_ptr<Iterator> > *children,
                                   int level, uint64_t number, bool drop_tombstones,
                                   FileMetaData *meta);

private:
    mutable std::mutex mutex_;
    bool opened_;
    std::string root_path_;
    DBOptions options_;
    std::string last_error_;
    int lock_fd_;

    Version version_;
    MemTable active_;
    std::map<uint64_t, std::unique_ptr<TableReader> > readers_;
    std::unique_ptr<WalWriter> wal_writer_;

    uint64_t last_sequence_;
    uint64_t next_file_number_;
    uint64_t log_number_;
};

} // namespace wedb

#endif
