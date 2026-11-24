#include "iterator.h"
#include "comparator.h"

namespace wedb {

MergeIterator::MergeIterator(std::vector<std::unique_ptr<Iterator> > children)
: children_(std::move(children)),
  valid_(false)
{
    pick_current();
}

void MergeIterator::seek(const std::string &key)
{
    for (size_t i = 0; i < children_.size(); ++i) {
        if (children_[i].get() != nullptr) {
            children_[i]->seek(key);
        }
    }
    pick_current();
}

void MergeIterator::next(void)
{
    if (!valid_) {
        return;
    }

    const std::string key = current_.key();
    for (size_t i = 0; i < children_.size(); ++i) {
        if (children_[i].get() != nullptr &&
            children_[i]->valid() &&
            children_[i]->entry().key() == key) {
            children_[i]->next();
        }
    }
    pick_current();
}

void MergeIterator::pick_current(void)
{
    valid_ = false;

    BytewiseComparator compare;
    Iterator *best = nullptr;
    for (size_t i = 0; i < children_.size(); ++i) {
        if (children_[i].get() == nullptr || !children_[i]->valid()) {
            continue;
        }
        if (best == nullptr) {
            best = children_[i].get();
            continue;
        }

        int ret = compare.compare(children_[i]->entry().key(), best->entry().key());
        if (ret < 0 ||
            (ret == 0 && children_[i]->entry().seq() > best->entry().seq())) {
            best = children_[i].get();
        }
    }

    if (best != nullptr) {
        current_ = best->entry();
        valid_ = true;
    }
}

} // namespace wedb
