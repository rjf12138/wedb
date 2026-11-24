#ifndef __WEDB_BLOOM_FILTER_H__
#define __WEDB_BLOOM_FILTER_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace wedb {

// 动态 BloomFilter：构建阶段收集 hash，build() 时分配位图。
// 序列化格式：bits_per_key(4) | k(4) | bit_count(8) | bits | crc32(4)
class BloomFilter {
public:
    explicit BloomFilter(uint32_t bits_per_key = 10);
    ~BloomFilter(void);

    void clear(void);
    void add(const std::string &key);
    void build(void);

    bool may_contain(const std::string &key) const;

    bool encode(std::string *out) const;
    bool decode(const char *data, size_t size, size_t *consumed);

    uint32_t bits_per_key(void) const { return bits_per_key_; }
    uint32_t k(void) const { return k_; }
    uint64_t bit_count(void) const { return bit_count_; }
    bool built(void) const { return built_; }

private:
    static void hash_key(const std::string &key, uint32_t *h1, uint32_t *h2);

private:
    uint32_t bits_per_key_;
    uint32_t k_;
    uint64_t bit_count_;
    std::vector<uint64_t> bits_;
    std::vector<std::pair<uint32_t, uint32_t> > hashes_;
    bool built_;
};

} // namespace wedb

#endif
