#include "block.h"
#include "coding.h"
#include "comparator.h"

namespace wedb {

BlockBuilder::BlockBuilder(void)
{
}

BlockBuilder::~BlockBuilder(void)
{
}

bool BlockBuilder::add(const Entry &entry)
{
    std::string encoded;
    if (!entry.encode(&encoded, false)) {
        return false;
    }
    buffer_.append(encoded);
    return true;
}

bool BlockBuilder::finish(std::string *block) const
{
    if (block == nullptr) {
        return false;
    }
    *block = buffer_;
    uint32_t checksum = crc32(buffer_.data(), buffer_.size());
    put_fixed32(block, checksum);
    return true;
}

void BlockBuilder::clear(void)
{
    buffer_.clear();
}

BlockReader::BlockReader(void)
{
}

BlockReader::~BlockReader(void)
{
}

bool BlockReader::parse(const std::string &block)
{
    entries_.clear();
    if (block.size() < 4) {
        return false;
    }

    size_t payload_size = block.size() - 4;
    size_t pos = payload_size;
    uint32_t expected = 0;
    if (!get_fixed32(block.data(), block.size(), &pos, &expected)) {
        return false;
    }
    if (crc32(block.data(), payload_size) != expected) {
        return false;
    }

    size_t read_pos = 0;
    while (read_pos < payload_size) {
        Entry entry;
        size_t consumed = 0;
        if (!entry.decode(block.data() + read_pos, payload_size - read_pos, &consumed)) {
            entries_.clear();
            return false;
        }
        if (consumed == 0) {
            entries_.clear();
            return false;
        }
        entries_.push_back(entry);
        read_pos += consumed;
    }
    return read_pos == payload_size;
}

bool BlockReader::get(const std::string &key, Entry *entry) const
{
    if (entry == nullptr) {
        return false;
    }
    BytewiseComparator compare;
    for (size_t i = 0; i < entries_.size(); ++i) {
        int ret = compare.compare(entries_[i].key(), key);
        if (ret == 0) {
            *entry = entries_[i];
            return true;
        }
        if (ret > 0) {
            return false;
        }
    }
    return false;
}

void BlockReader::clear(void)
{
    entries_.clear();
}

} // namespace wedb
