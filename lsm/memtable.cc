#include "memtable.h"

namespace wedb {

namespace {
const size_t kEntryOverhead = 64;
}

MemTable::MemTable(void)
: table_(new Table()),
  memory_usage_(0),
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

    Table::Iterator iter = table_->get(entry.key());
    if (iter == table_->end()) {
        memory_usage_ += entry.key().size() + entry.value().size() + kEntryOverhead;
    } else {
        memory_usage_ -= iter.value().value().size();
        memory_usage_ += entry.value().size();
    }

    table_->put(entry.key(), entry);
    if (entry.seq() > max_seq_) {
        max_seq_ = entry.seq();
    }
    return true;
}

bool MemTable::get(const std::string &key, Entry *entry)
{
    if (entry == nullptr) {
        return false;
    }
    Table::Iterator iter = table_->get(key);
    if (iter == table_->end()) {
        return false;
    }
    *entry = iter.value();
    return true;
}

void MemTable::clear(void)
{
    table_->clear();
    memory_usage_ = 0;
    max_seq_ = 0;
}

std::unique_ptr<Iterator> MemTable::new_iterator(void)
{
    return std::unique_ptr<Iterator>(new MemTableIterator(table_.get()));
}

MemTableIterator::MemTableIterator(MemTable::Table *table)
: table_(table),
  iter_(table != nullptr ? table->begin() : MemTable::Table::Iterator()),
  valid_(false)
{
    if (table_ != nullptr) {
        valid_ = (iter_ != table_->end());
    }
}

void MemTableIterator::seek(const std::string &key)
{
    if (table_ == nullptr) {
        valid_ = false;
        return;
    }

    BytewiseComparator compare;
    iter_ = table_->begin();
    while (iter_ != table_->end() && compare.compare(iter_.key(), key) < 0) {
        ++iter_;
    }
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
