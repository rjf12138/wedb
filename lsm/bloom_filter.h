#ifndef __BLOOM_FILTER_H__
#define __BLOOM_FILTER_H__

#include <cstdint>

class BloomFilter {
public:
    BloomFilter(void);
    ~BloomFilter(void);

    // 添加值
    void push(int key);
    // 清空所有值
    void clear(void) {bitset_ = 0x00;}
    // 检查值是否存在
    bool exist(int key);
    
    // 测试打印
    void print(int key);
    
private:
    uint32_t bitset_;
};

#endif