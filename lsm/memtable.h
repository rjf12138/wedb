#ifndef __WEDB_MEMTABLE_H__
#define __WEDB_MEMTABLE_H__

#include "comparator.h"
#include "entry.h"
#include "iterator.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace wedb {

// v1 用 std::map 做内存表：行为等价于有序红黑树，简单且没有 SkipList 的边界风险。
class MemTable {
public:
    typedef std::map<std::string, Entry, BytewiseComparator> Table;

    MemTable(void);
    ~MemTable(void);

    bool add(const Entry &entry);
    bool get(const std::string &key, Entry *entry) const;

    bool empty(void) const { return table_.empty(); }
    size_t size(void) const { return table_.size(); }
    size_t approximate_memory_usage(void) const { return memory_usage_; }
    uint64_t max_seq(void) const { return max_seq_; }
    void clear(void);

    const Table &entries(void) const { return table_; }

private:
    Table table_;
    size_t memory_usage_;
    uint64_t max_seq_;
};

class MemTableIterator : public Iterator {
public:
    explicit MemTableIterator(const MemTable::Table *table);
    virtual ~MemTableIterator(void) {}

    void seek(const std::string &key) override;
    void next(void) override;
    bool valid(void) const override { return valid_; }
    const Entry &entry(void) const override { return iter_->second; }

private:
    const MemTable::Table *table_;
    MemTable::Table::const_iterator iter_;
    bool valid_;
};

} // namespace wedb

#endif
