#ifndef __WEDB_MANIFEST_H__
#define __WEDB_MANIFEST_H__

#include "version.h"

#include <cstdint>
#include <string>

namespace wedb {

// 行式文本 Manifest（避免 JSON 数字精度的坑）：
//   wedb_manifest 1
//   last_sequence <n>
//   next_file_number <n>
//   log_number <n>
//   file <level> <number> <size> <max_seq> <smallest_hex> <largest_hex>
bool load_manifest(const std::string &path, int num_levels,
                   Version *version, uint64_t *last_sequence,
                   uint64_t *next_file_number, uint64_t *log_number);

bool save_manifest_atomic(const std::string &path, const Version &version,
                          uint64_t last_sequence, uint64_t next_file_number,
                          uint64_t log_number);

} // namespace wedb

#endif
