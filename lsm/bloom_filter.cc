#include "bloom_filter.h"
#include "algorithm/algorithm.h"

BloomFilter::BloomFilter(void)
:bitset_(0)
{

}
    
BloomFilter::~BloomFilter(void)
{

}

void 
BloomFilter::push(const basic::ByteBuffer &buffer)
{
    ssize_t size = buffer.data_size();
    char *key = new char[size];

    uint32_t h1 = 0, h2 = 0, h3 = 0, h4 = 0;
    algorithm::murmurhash3_x86_32(key, size, 0x234, &h1);
    algorithm::murmurhash3_x86_32(key, size, 0x345, &h2);
    algorithm::murmurhash3_x86_32(key, size, 0x456, &h3);
    algorithm::murmurhash3_x86_32(key, size, 0x678, &h4);

    bitset_ = bitset_ | (0x01 << (h1 % 32));
    bitset_ = bitset_ | (0x01 << (h2 % 32));
    bitset_ = bitset_ | (0x01 << (h3 % 32));
    bitset_ = bitset_ | (0x01 << (h4 % 32));

    delete []key;
}

bool 
BloomFilter::exist(const basic::ByteBuffer &buffer)
{
    ssize_t size = buffer.data_size();
    char *key = new char[size];

    uint32_t h1 = 0, h2 = 0, h3 = 0, h4 = 0;
    algorithm::murmurhash3_x86_32(key, size, 0x234, &h1);
    algorithm::murmurhash3_x86_32(key, size, 0x345, &h2);
    algorithm::murmurhash3_x86_32(key, size, 0x456, &h3);
    algorithm::murmurhash3_x86_32(key, size, 0x678, &h4);

    bool ret1 = bitset_ & (0x01 << (h1 % 32));
    bool ret2 = bitset_ & (0x01 << (h2 % 32));
    bool ret3 = bitset_ & (0x01 << (h3 % 32));
    bool ret4 = bitset_ & (0x01 << (h4 % 32));

    delete []key;

    return (ret1 && ret2 && ret3 && ret4);
}

void 
BloomFilter::print(const basic::ByteBuffer &buffer)
{
    ssize_t size = buffer.data_size();
    char *key = new char[size];

    uint32_t h1 = 0, h2 = 0, h3 = 0, h4 = 0;
    algorithm::murmurhash3_x86_32(key, size, 0x234, &h1);
    algorithm::murmurhash3_x86_32(key, size, 0x345, &h2);
    algorithm::murmurhash3_x86_32(key, size, 0x456, &h3);
    algorithm::murmurhash3_x86_32(key, size, 0x678, &h4);

    fprintf(stdout, "h1=%d, h2=%d, h3=%d, h4=%d\n", h1 % 32, h2 % 32, h3 % 32, h4 % 32);

    bitset_ = bitset_ | (0x01 << (h1 % 32));
    bitset_ = bitset_ | (0x01 << (h2 % 32));
    bitset_ = bitset_ | (0x01 << (h3 % 32));
    bitset_ = bitset_ | (0x01 << (h4 % 32));

    for (uint32_t i = 0; i < 32; ++i) {
        fprintf(stdout, "%d ", (bitset_ & (0x01 << i)) != 0 ? 1 : 0);
    }
    fprintf(stdout, "\n");

    delete []key;                  
}