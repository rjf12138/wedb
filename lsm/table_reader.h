#ifndef __WEDB_TABLE_READER_H__
#define __WEDB_TABLE_READER_H__

#include "bloom_filter.h"
#include "entry.h"
#include "env.h"
#include "iterator.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace wedb {

class TableReader;

class TableIterator : public Iterator {
public:
    explicit TableIterator(const TableReader *reader);
    virtual ~TableIterator(void) {}

    void seek(const std::string &key) override;
    void next(void) override;
    bool valid(void) const override { return valid_; }
    const Entry &entry(void) const override { return current_; }

private:
    bool load_block(size_t block_index);
    bool advance_to_next_block(void);

private:
    const TableReader *reader_;
    size_t block_index_;
    size_t pos_;
    std::vector<Entry> entries_;
    Entry current_;
    bool valid_;
};

class TableReader {
public:
    friend class TableIterator;

    struct IndexEntry {
        std::string last_key;
        uint64_t offset;
        uint32_t size;

        IndexEntry(void) : offset(0), size(0) {}
    };

    TableReader(void);
    ~TableReader(void);

    bool open(const std::string &path, uint64_t number);
    bool get(const std::string &key, Entry *entry) const;

    const std::vector<IndexEntry> &index(void) const { return index_; }
    const BloomFilter &filter(void) const { return filter_; }
    bool filter_valid(void) const { return filter_valid_; }
    bool corrupted(void) const { return corrupted_; }

    uint64_t number(void) const { return number_; }
    uint64_t file_size(void) const { return file_size_; }

    std::unique_ptr<Iterator> new_iterator(void) const;

private:
    bool read_block(size_t block_index, std::vector<Entry> *entries) const;

private:
    TableReader(const TableReader &);
    TableReader &operator=(const TableReader &);

private:
    ReadableFile file_;
    std::vector<IndexEntry> index_;
    BloomFilter filter_;
    bool filter_valid_;
    mutable bool corrupted_;
    uint64_t number_;
    uint64_t file_size_;
};

} // namespace wedb

#endif
