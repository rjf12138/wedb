#ifndef __WEDB_ENTRY_H__
#define __WEDB_ENTRY_H__

#include <cstddef>
#include <cstdint>
#include <string>

namespace wedb {

enum EntryType {
    EntryType_Put = 0,
    EntryType_Delete = 1,
};

// 单条键值记录。WAL 和 SSTable 共用同一套编码。
class Entry {
public:
    Entry(void);
    Entry(uint64_t seq, uint8_t type, const std::string &key, const std::string &value);

    // 序列化。include_crc=true 时 Entry 自带 CRC32（主要用于测试）。
    bool encode(std::string *out, bool include_crc = false) const;

    // 从 data 解出一条 Entry。成功后 consumed 返回消耗的字节数。
    bool decode(const char *data, size_t size, size_t *consumed);

    size_t encoded_size(bool include_crc = false) const;

    uint64_t seq(void) const { return seq_; }
    uint8_t type(void) const { return type_; }
    const std::string &key(void) const { return key_; }
    const std::string &value(void) const { return value_; }

    void set_seq(uint64_t seq) { seq_ = seq; }
    void set_type(uint8_t type) { type_ = type; }
    void set_key(const std::string &key) { key_ = key; }
    void set_value(const std::string &value) { value_ = value; }

    bool is_delete(void) const { return type_ == EntryType_Delete; }

private:
    uint64_t seq_;
    uint8_t type_;
    std::string key_;
    std::string value_;
};

} // namespace wedb

#endif
