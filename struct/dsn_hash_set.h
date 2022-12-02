#ifndef _DYNAMIC_SIZED_V2__
#define _DYNAMIC_SIZED_V2__

#include "FSet.h"

// 动态分配的哈希集合
class DSHashSet {
public:
    DSHashSet(void);
    ~DSHashSet(void);

    // 插入元素
    bool insert(const int &key);
    // 移除元素
    bool remove(const int &key);
    // 元素是否存在
    bool exist(const int &key);
    // 元素数量
    uint32_t size(void);

    void print(void);

private:
    bool apply(FSetOp op);
    uint32_t resize(bool grow);

private:
    uint32_t elem_size_;        //插入元素数量

    FSetArray *curr_buckets_;
    FSetArray *pred_buckets_;

    unsigned max_data_size_;
};

#endif