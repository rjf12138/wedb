#ifndef __BLOOM_FILTER_H__
#define __BLOOM_FILTER_H__

#include <cstdint>
#include "basic/byte_buffer.h"

class BloomFilter {
public:
    BloomFilter(void);
    ~BloomFilter(void);

    // 添加值
    void push(basic::ByteBuffer &buffer);
    // 清空所有值
    void clear(void) {bitset_ = 0x00;}
    // 检查值是否存在
    bool exist(basic::ByteBuffer &buffer);
    // 返回布隆过滤器结果
    int result(void) {return bitset_;}
    
    // 测试打印
    void print(basic::ByteBuffer &buffer);
    
private:
    int bitset_;
};

#endif