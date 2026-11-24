#include "coding.h"

#include <cstring>

namespace wedb {

namespace {

uint32_t crc32_table[256];
bool crc32_table_ready = false;

void init_crc32_table(void)
{
    if (crc32_table_ready) {
        return;
    }

    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_ready = true;
}

uint32_t crc32_impl(uint32_t crc, const uint8_t *data, size_t size)
{
    init_crc32_table();
    for (size_t i = 0; i < size; ++i) {
        crc = crc32_table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8);
    }
    return crc;
}

} // namespace

void put_fixed16(std::string *dst, uint16_t value)
{
    dst->push_back(static_cast<char>(value & 0xFFU));
    dst->push_back(static_cast<char>((value >> 8) & 0xFFU));
}

void put_fixed24(std::string *dst, uint32_t value)
{
    dst->push_back(static_cast<char>(value & 0xFFU));
    dst->push_back(static_cast<char>((value >> 8) & 0xFFU));
    dst->push_back(static_cast<char>((value >> 16) & 0xFFU));
}

void put_fixed32(std::string *dst, uint32_t value)
{
    dst->push_back(static_cast<char>(value & 0xFFU));
    dst->push_back(static_cast<char>((value >> 8) & 0xFFU));
    dst->push_back(static_cast<char>((value >> 16) & 0xFFU));
    dst->push_back(static_cast<char>((value >> 24) & 0xFFU));
}

void put_fixed64(std::string *dst, uint64_t value)
{
    for (int i = 0; i < 8; ++i) {
        dst->push_back(static_cast<char>((value >> (8 * i)) & 0xFFU));
    }
}

bool get_fixed16(const char *data, size_t size, size_t *pos, uint16_t *value)
{
    if (*pos + 2 > size) {
        return false;
    }
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data + *pos);
    *value = static_cast<uint16_t>(p[0]) |
             (static_cast<uint16_t>(p[1]) << 8);
    *pos += 2;
    return true;
}

bool get_fixed24(const char *data, size_t size, size_t *pos, uint32_t *value)
{
    if (*pos + 3 > size) {
        return false;
    }
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data + *pos);
    *value = static_cast<uint32_t>(p[0]) |
             (static_cast<uint32_t>(p[1]) << 8) |
             (static_cast<uint32_t>(p[2]) << 16);
    *pos += 3;
    return true;
}

bool get_fixed32(const char *data, size_t size, size_t *pos, uint32_t *value)
{
    if (*pos + 4 > size) {
        return false;
    }
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data + *pos);
    *value = static_cast<uint32_t>(p[0]) |
             (static_cast<uint32_t>(p[1]) << 8) |
             (static_cast<uint32_t>(p[2]) << 16) |
             (static_cast<uint32_t>(p[3]) << 24);
    *pos += 4;
    return true;
}

bool get_fixed64(const char *data, size_t size, size_t *pos, uint64_t *value)
{
    if (*pos + 8 > size) {
        return false;
    }
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data + *pos);
    *value = static_cast<uint64_t>(p[0]) |
             (static_cast<uint64_t>(p[1]) << 8) |
             (static_cast<uint64_t>(p[2]) << 16) |
             (static_cast<uint64_t>(p[3]) << 24) |
             (static_cast<uint64_t>(p[4]) << 32) |
             (static_cast<uint64_t>(p[5]) << 40) |
             (static_cast<uint64_t>(p[6]) << 48) |
             (static_cast<uint64_t>(p[7]) << 56);
    *pos += 8;
    return true;
}

uint32_t crc32_extend(uint32_t crc, const void *data, size_t size)
{
    uint32_t value = crc ^ 0xFFFFFFFFU;
    value = crc32_impl(value, reinterpret_cast<const uint8_t *>(data), size);
    return value ^ 0xFFFFFFFFU;
}

uint32_t crc32(const void *data, size_t size)
{
    return crc32_extend(0, data, size);
}

std::string hex_encode(const std::string &input)
{
    static const char *digits = "0123456789abcdef";
    std::string output;
    output.reserve(input.size() * 2);
    for (size_t i = 0; i < input.size(); ++i) {
        uint8_t value = static_cast<uint8_t>(input[i]);
        output.push_back(digits[(value >> 4) & 0x0F]);
        output.push_back(digits[value & 0x0F]);
    }
    return output;
}

bool hex_decode(const std::string &input, std::string *output)
{
    if ((input.size() % 2) != 0) {
        return false;
    }

    output->clear();
    output->reserve(input.size() / 2);
    for (size_t i = 0; i < input.size(); i += 2) {
        int high = 0;
        int low = 0;
        for (int j = 0; j < 2; ++j) {
            char c = input[i + j];
            int value = 0;
            if (c >= '0' && c <= '9') {
                value = c - '0';
            } else if (c >= 'a' && c <= 'f') {
                value = c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                value = c - 'A' + 10;
            } else {
                return false;
            }
            if (j == 0) {
                high = value;
            } else {
                low = value;
            }
        }
        output->push_back(static_cast<char>((high << 4) | low));
    }
    return true;
}

} // namespace wedb
