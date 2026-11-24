#include "entry.h"
#include "coding.h"

#include <cstring>

namespace wedb {

namespace {

const uint32_t kMaxKeySize = 0xFFFFU;
const uint32_t kMaxValueSize = 0xFFFFFFU;

uint8_t key_length_width(size_t key_size)
{
    return key_size <= 0xFFU ? 1 : 2;
}

uint8_t value_length_width(size_t value_size)
{
    if (value_size <= 0xFFU) {
        return 1;
    }
    if (value_size <= 0xFFFFU) {
        return 2;
    }
    return 3;
}

void put_length(std::string *dst, uint32_t value, uint8_t width)
{
    if (width == 1) {
        dst->push_back(static_cast<char>(value & 0xFFU));
    } else if (width == 2) {
        put_fixed16(dst, static_cast<uint16_t>(value));
    } else {
        put_fixed24(dst, value);
    }
}

bool get_length(const char *data, size_t limit, size_t *pos, uint8_t width, uint32_t *value)
{
    if (width == 1) {
        if (*pos + 1 > limit) {
            return false;
        }
        *value = static_cast<uint8_t>(data[*pos]);
        *pos += 1;
        return true;
    }
    if (width == 2) {
        uint16_t v = 0;
        if (!get_fixed16(data, limit, pos, &v)) {
            return false;
        }
        *value = v;
        return true;
    }
    if (width == 3) {
        return get_fixed24(data, limit, pos, value);
    }
    return false;
}

} // namespace

Entry::Entry(void)
: seq_(0),
  type_(EntryType_Put)
{
}

Entry::Entry(uint64_t seq, uint8_t type, const std::string &key, const std::string &value)
: seq_(seq),
  type_(type),
  key_(key),
  value_(value)
{
}

size_t Entry::encoded_size(bool include_crc) const
{
    size_t key_width = key_length_width(key_.size());
    size_t value_width = value_length_width(value_.size());
    size_t body_size = key_width + key_.size() + 8 + value_width + value_.size();
    return 1 + 4 + body_size + (include_crc ? 4 : 0);
}

bool Entry::encode(std::string *out, bool include_crc) const
{
    if (out == nullptr) {
        return false;
    }
    if (key_.size() > kMaxKeySize || value_.size() > kMaxValueSize) {
        return false;
    }
    if (type_ != EntryType_Put && type_ != EntryType_Delete) {
        return false;
    }

    uint8_t key_width = key_length_width(key_.size());
    uint8_t value_width = value_length_width(value_.size());
    uint8_t mark = 0;
    if (key_width == 2) {
        mark |= (1U << 1);
    }
    mark |= static_cast<uint8_t>(value_width << 2);
    mark |= static_cast<uint8_t>((type_ & 0x03U) << 6);
    if (include_crc) {
        mark |= (1U << 5);
    }

    std::string body;
    body.reserve(key_width + key_.size() + 8 + value_width + value_.size());
    put_length(&body, static_cast<uint32_t>(key_.size()), key_width);
    body.append(key_);
    put_fixed64(&body, seq_);
    put_length(&body, static_cast<uint32_t>(value_.size()), value_width);
    body.append(value_);

    out->clear();
    out->reserve(1 + 4 + body.size() + (include_crc ? 4 : 0));
    out->push_back(static_cast<char>(mark));
    put_fixed32(out, static_cast<uint32_t>(body.size()));
    out->append(body);

    if (include_crc) {
        uint32_t checksum = crc32(out->data(), out->size());
        put_fixed32(out, checksum);
    }
    return true;
}

bool Entry::decode(const char *data, size_t size, size_t *consumed)
{
    if (data == nullptr || consumed == nullptr || size < 5) {
        return false;
    }

    size_t pos = 0;
    uint8_t mark = static_cast<uint8_t>(data[pos++]);
    if ((mark & (1U << 4)) != 0) {
        return false; // v1 不支持压缩
    }

    uint32_t record_len = 0;
    if (!get_fixed32(data, size, &pos, &record_len)) {
        return false;
    }
    if (record_len > size - pos) {
        return false;
    }
    size_t body_end = pos + record_len;

    uint8_t key_width = ((mark >> 1) & 0x01U) ? 2 : 1;
    uint32_t key_size = 0;
    if (!get_length(data, body_end, &pos, key_width, &key_size)) {
        return false;
    }
    if (key_size > body_end - pos) {
        return false;
    }
    std::string key(data + pos, key_size);
    pos += key_size;

    uint64_t seq = 0;
    if (!get_fixed64(data, body_end, &pos, &seq)) {
        return false;
    }

    uint8_t value_width = static_cast<uint8_t>((mark >> 2) & 0x03U);
    if (value_width == 0) {
        return false;
    }
    uint32_t value_size = 0;
    if (!get_length(data, body_end, &pos, value_width, &value_size)) {
        return false;
    }
    if (value_size > body_end - pos) {
        return false;
    }
    std::string value(data + pos, value_size);
    pos += value_size;

    if ((mark & 0x01U) != 0) {
        if (pos >= body_end) {
            return false;
        }
        uint8_t extend_size = static_cast<uint8_t>(data[pos++]);
        if (extend_size > body_end - pos) {
            return false;
        }
        pos += extend_size;
    }

    if (pos != body_end) {
        return false;
    }

    if ((mark & (1U << 5)) != 0) {
        uint32_t expected = 0;
        if (!get_fixed32(data, size, &pos, &expected)) {
            return false;
        }
        uint32_t actual = crc32(data, body_end);
        if (actual != expected) {
            return false;
        }
    }

    uint8_t type = static_cast<uint8_t>((mark >> 6) & 0x03U);
    if (type != EntryType_Put && type != EntryType_Delete) {
        return false;
    }

    seq_ = seq;
    type_ = type;
    key_.swap(key);
    value_.swap(value);
    *consumed = pos;
    return true;
}

} // namespace wedb
