#ifndef __WEDB_CODING_H__
#define __WEDB_CODING_H__

#include <cstddef>
#include <cstdint>
#include <string>

namespace wedb {

// 小端固定宽度编码
void put_fixed16(std::string *dst, uint16_t value);
void put_fixed24(std::string *dst, uint32_t value);
void put_fixed32(std::string *dst, uint32_t value);
void put_fixed64(std::string *dst, uint64_t value);

bool get_fixed16(const char *data, size_t size, size_t *pos, uint16_t *value);
bool get_fixed24(const char *data, size_t size, size_t *pos, uint32_t *value);
bool get_fixed32(const char *data, size_t size, size_t *pos, uint32_t *value);
bool get_fixed64(const char *data, size_t size, size_t *pos, uint64_t *value);

// CRC32（IEEE 多项式），支持分块更新
uint32_t crc32(const void *data, size_t size);
uint32_t crc32_extend(uint32_t crc, const void *data, size_t size);

// 二进制 key 的十六进制表示，用于文本 Manifest
std::string hex_encode(const std::string &input);
bool hex_decode(const std::string &input, std::string *output);

} // namespace wedb

#endif
