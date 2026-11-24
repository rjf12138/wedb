#include "memtable.h"

namespace wedb {

namespace {
const size_t kEntryOverhead = 64;
}

MemTable::MemTable(void)
: memory_usage_(0),
  max_seq_(0)
{
}

MemTable::~MemTable(void)
{
}

bool MemTable::add(const Entry &entry)
{
    if (entry.key().empty()) {
        return false;
    }

    Table::iterator iter = table_.find(entry.key());
    if (iter == table_.end()) {
        memory_usage_ += entry.key().size() + entry.value().size() + kEntryOverhead;
    } else {
        memory_usage_ -= iter->second.key().size() + iter->second.value().size();
        memory_usage_ += entry.key().size() + entry.value().size();
    }

    table_[entry.key()] = entry;
    if (entry.seq() > max_seq_) {
        max_seq_ = entry.seq();
    }
    return true;
}

bool MemTable::get(const std::string &key, Entry *entry) const
{
    if (entry == nullptr) {
        return false;
    }
    Table::const_iterator iter = table_.find(key);
    if (iter == table_.end()) {
        return false;
    }
    *entry = iter->second;
    return true;
}

void MemTable::clear(void)
{
    table_.clear();
    memory_usage_ = 0;
    max_seq_ = 0;
}

MemTableIterator::MemTableIterator(const MemTable::Table *table)
: table_(table),
  iter_(table != nullptr ? table->end() : MemTable::Table::const_iterator()),
  valid_(false)
{
}

void MemTableIterator::seek(const std::string &key)
{
    if (table_ == nullptr) {
        valid_ = false;
        return;
    }
    iter_ = table_->lower_bound(key);
    valid_ = (iter_ != table_->end());
}

void MemTableIterator::next(void)
{
    if (table_ == nullptr || !valid_) {
        return;
    }
    ++iter_;
    valid_ = (iter_ != table_->end());
}

} // namespace wedb
