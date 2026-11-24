#include "bloom_filter.h"
#include "coding.h"

#include "algorithm/algorithm.h"

namespace wedb {

namespace {
const uint32_t kDefaultHashSeed1 = 0xBC9F1D34U;
const uint32_t kDefaultHashSeed2 = 0x9E3779B9U;
}

BloomFilter::BloomFilter(uint32_t bits_per_key)
: bits_per_key_(bits_per_key == 0 ? 10 : bits_per_key),
  k_(0),
  bit_count_(0),
  built_(false)
{
}

BloomFilter::~BloomFilter(void)
{
}

void BloomFilter::clear(void)
{
    bits_.clear();
    hashes_.clear();
    bit_count_ = 0;
    k_ = 0;
    built_ = false;
}

void BloomFilter::add(const std::string &key)
{
    uint32_t h1 = 0;
    uint32_t h2 = 0;
    hash_key(key, &h1, &h2);
    hashes_.push_back(std::make_pair(h1, h2));
    built_ = false;
}

void BloomFilter::hash_key(const std::string &key, uint32_t *h1, uint32_t *h2)
{
    algorithm::murmurhash3_x86_32(key.data(), static_cast<int>(key.size()), kDefaultHashSeed1, h1);
    algorithm::murmurhash3_x86_32(key.data(), static_cast<int>(key.size()), kDefaultHashSeed2, h2);
    if (*h2 == 0) {
        *h2 = 0x27D4EB2FU;
    }
}

void BloomFilter::build(void)
{
    if (built_) {
        return;
    }

    uint64_t bits = static_cast<uint64_t>(hashes_.size()) * bits_per_key_;
    if (bits < 64) {
        bits = 64;
    }
    if ((bits % 64) != 0) {
        bits = ((bits / 64) + 1) * 64;
    }

    bit_count_ = bits;
    k_ = static_cast<uint32_t>(bits_per_key_ * 0.69);
    if (k_ < 1) {
        k_ = 1;
    }
    if (k_ > 30) {
        k_ = 30;
    }

    bits_.assign(static_cast<size_t>(bit_count_ / 64), 0);
    for (size_t i = 0; i < hashes_.size(); ++i) {
        uint32_t h1 = hashes_[i].first;
        uint32_t h2 = hashes_[i].second;
        for (uint32_t j = 0; j < k_; ++j) {
            uint64_t bit = static_cast<uint64_t>(h1) + static_cast<uint64_t>(j) * h2;
            bit %= bit_count_;
            bits_[static_cast<size_t>(bit / 64)] |= (1ULL << (bit % 64));
        }
    }
    built_ = true;
}

bool BloomFilter::may_contain(const std::string &key) const
{
    if (!built_) {
        return true;
    }
    if (bit_count_ == 0 || bits_.empty()) {
        return false;
    }

    uint32_t h1 = 0;
    uint32_t h2 = 0;
    hash_key(key, &h1, &h2);
    for (uint32_t j = 0; j < k_; ++j) {
        uint64_t bit = static_cast<uint64_t>(h1) + static_cast<uint64_t>(j) * h2;
        bit %= bit_count_;
        if ((bits_[static_cast<size_t>(bit / 64)] & (1ULL << (bit % 64))) == 0) {
            return false;
        }
    }
    return true;
}

bool BloomFilter::encode(std::string *out) const
{
    if (out == nullptr) {
        return false;
    }
    if (!built_) {
        const_cast<BloomFilter *>(this)->build();
    }

    out->clear();
    put_fixed32(out, bits_per_key_);
    put_fixed32(out, k_);
    put_fixed64(out, bit_count_);
    for (size_t i = 0; i < bits_.size(); ++i) {
        put_fixed64(out, bits_[i]);
    }
    uint32_t checksum = crc32(out->data(), out->size());
    put_fixed32(out, checksum);
    return true;
}

bool BloomFilter::decode(const char *data, size_t size, size_t *consumed)
{
    if (data == nullptr || consumed == nullptr || size < 16) {
        return false;
    }

    size_t pos = 0;
    uint32_t bits_per_key = 0;
    uint32_t k = 0;
    uint64_t bit_count = 0;
    if (!get_fixed32(data, size, &pos, &bits_per_key) ||
        !get_fixed32(data, size, &pos, &k) ||
        !get_fixed64(data, size, &pos, &bit_count)) {
        return false;
    }

    if (bits_per_key == 0 || k == 0 || k > 30 || bit_count == 0 || (bit_count % 64) != 0) {
        return false;
    }
    // 防御损坏文件导致超大内存分配（最多 2GB bits，约 256MB 内存）
    if (bit_count > (1ULL << 34)) {
        return false;
    }

    uint64_t word_count = bit_count / 64;
    uint64_t bytes_needed = word_count * 8;
    if (bytes_needed > size - pos || size - pos < bytes_needed + 4) {
        return false;
    }

    std::vector<uint64_t> bits;
    bits.reserve(static_cast<size_t>(word_count));
    for (uint64_t i = 0; i < word_count; ++i) {
        uint64_t value = 0;
        if (!get_fixed64(data, size, &pos, &value)) {
            return false;
        }
        bits.push_back(value);
    }

    uint32_t expected = 0;
    if (!get_fixed32(data, size, &pos, &expected)) {
        return false;
    }
    if (crc32(data, pos - 4) != expected) {
        return false;
    }

    bits_per_key_ = bits_per_key;
    k_ = k;
    bit_count_ = bit_count;
    bits_.swap(bits);
    hashes_.clear();
    built_ = true;
    *consumed = pos;
    return true;
}

} // namespace wedb
