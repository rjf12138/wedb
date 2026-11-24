#ifndef __WEDB_BLOCK_H__
#define __WEDB_BLOCK_H__

#include "entry.h"

#include <cstddef>
#include <string>
#include <vector>

namespace wedb {

// data block：若干 Entry + 末尾 crc32(4)
class BlockBuilder {
public:
    BlockBuilder(void);
    ~BlockBuilder(void);

    bool add(const Entry &entry);
    bool finish(std::string *block) const;

    bool empty(void) const { return buffer_.empty(); }
    size_t size(void) const { return buffer_.size(); }
    void clear(void);

private:
    std::string buffer_;
};

class BlockReader {
public:
    BlockReader(void);
    ~BlockReader(void);

    bool parse(const std::string &block);
    bool get(const std::string &key, Entry *entry) const;

    const std::vector<Entry> &entries(void) const { return entries_; }
    void clear(void);

private:
    std::vector<Entry> entries_;
};

} // namespace wedb

#endif
