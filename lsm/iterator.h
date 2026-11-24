#ifndef __WEDB_ITERATOR_H__
#define __WEDB_ITERATOR_H__

#include "entry.h"

#include <memory>
#include <string>
#include <vector>

namespace wedb {

class Iterator {
public:
    virtual ~Iterator(void) {}
    virtual void seek(const std::string &key) = 0;
    virtual void next(void) = 0;
    virtual bool valid(void) const = 0;
    virtual const Entry &entry(void) const = 0;
};

// 多路归并。同一 key 取 seq 最大的 Entry；next() 会一次性跳过所有同 key 版本。
class MergeIterator : public Iterator {
public:
    explicit MergeIterator(std::vector<std::unique_ptr<Iterator> > children);
    virtual ~MergeIterator(void) {}

    void seek(const std::string &key) override;
    void next(void) override;
    bool valid(void) const override { return valid_; }
    const Entry &entry(void) const override { return current_; }

private:
    void pick_current(void);

private:
    std::vector<std::unique_ptr<Iterator> > children_;
    Entry current_;
    bool valid_;
};

} // namespace wedb

#endif
