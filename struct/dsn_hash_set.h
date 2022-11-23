#ifndef _DYNAMIC_SIZED_V2__
#define _DYNAMIC_SIZED_V2__

#include "FSet.h"

// 动态分配的哈希集合
class DSHashSet {
public:
    DSHashSet(void);
    ~DSHashSet(void);

    // 返回错误分为三种，1. 成功，2. 插入/删除失败 3. 当前桶被冻结无法插入，要等一下才能插入
    // 如果错误是3,需要循环等待，返回值不要用bool，用enum表示那三种
    bool insert(const int &key);
    bool remove(const int &key);
    bool exist(const int &key);
    uint32_t size(void);

    // void print(void);

private:
    int when_resize_hash_table(void);
    bool apply(FSetOp op);
    uint32_t resize(bool grow);
    void init_buckets(int pos);

private:
    uint32_t elem_size_;        //插入元素数量

    FSetArray *curr_buckets_;
    FSetArray *pred_buckets_;
    // unsigned curr_bucket_size_;
    // FSet **buckets_ptr_;        // 当前的hash数据集

    // unsigned pred_bucket_size_;
    // FSet **pred_buckets_ptr_;   // 前置的hash数据集

    unsigned max_data_size_;
};

#endif