#ifndef __WEDB_MEMTABLE_H__
#define __WEDB_MEMTABLE_H__

#include "comparator.h"
#include "entry.h"
#include "iterator.h"

#include "data_structure/skip_list.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace wedb {

// ds::SkipList 要求三路比较（-1/0/1），不能直接用 std::map 的 bool 比较器。
struct MemTableComparator {
    int32_t operator()(const std::string &left, const std::string &right) const
    {
        BytewiseComparator compare;
        return static_cast<int32_t>(compare.compare(left, right));
    }
};

class MemTableIterator;

class MemTable {
public:
    typedef ds::SkipList<std::string, Entry, MemTableComparator> Table;

    MemTable(void);
    ~MemTable(void);

    bool add(const Entry &entry);
    bool get(const std::string &key, Entry *entry);

    bool empty(void) const { return table_->empty(); }
    size_t size(void) const { return static_cast<size_t>(table_->size()); }
    size_t approximate_memory_usage(void) const { return memory_usage_; }
    uint64_t max_seq(void) const { return max_seq_; }
    void clear(void);

    std::unique_ptr<Iterator> new_iterator(void);

private:
    MemTable(const MemTable &);
    MemTable &operator=(const MemTable &);

private:
    std::unique_ptr<Table> table_;
    size_t memory_usage_;
    uint64_t max_seq_;
};

class MemTableIterator : public Iterator {
public:
    explicit MemTableIterator(MemTable::Table *table);
    virtual ~MemTableIterator(void) {}

    void seek(const std::string &key) override;
    void next(void) override;
    bool valid(void) const override { return valid_; }
    const Entry &entry(void) const override { return iter_.value(); }

private:
    MemTable::Table *table_;
    mutable MemTable::Table::Iterator iter_;
    bool valid_;
};

} // namespace wedb

#endif
