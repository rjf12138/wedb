#ifndef __WEDB_TABLE_BUILDER_H__
#define __WEDB_TABLE_BUILDER_H__

#include "block.h"
#include "bloom_filter.h"
#include "env.h"
#include "entry.h"
#include "version.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wedb {

struct TableBuilderOptions {
    int level;
    uint64_t file_number;
    size_t block_size;
    uint32_t bits_per_key;

    TableBuilderOptions(void)
    : level(0), file_number(0), block_size(4096), bits_per_key(10) {}
};

class TableBuilder {
public:
    TableBuilder(void);
    ~TableBuilder(void);

    bool open(const std::string &final_path, const TableBuilderOptions &options);
    bool add(const Entry &entry);
    bool finish(FileMetaData *meta);

    bool started(void) const { return started_; }
    bool finished(void) const { return finished_; }
    uint64_t current_size(void) const { return offset_ + block_.size(); }

private:
    struct IndexEntry {
        std::string last_key;
        uint64_t offset;
        uint32_t size;

        IndexEntry(void) : offset(0), size(0) {}
    };

    bool flush_block(void);
    void cleanup(void);

private:
    TableBuilderOptions options_;
    std::string final_path_;
    std::string tmp_path_;
    WritableFile file_;
    BlockBuilder block_;
    BloomFilter bloom_;
    std::vector<IndexEntry> index_;
    std::string last_key_;
    std::string smallest_key_;
    std::string largest_key_;
    uint64_t max_seq_;
    uint64_t offset_;
    bool started_;
    bool finished_;
};

} // namespace wedb

#endif
