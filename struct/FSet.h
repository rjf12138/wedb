#ifndef __FSET_H__
#define __FSET_H__

#include "basic/basic_head.h"
#include "basic/logger.h"
#include "system/system.h"
#include "algorithm/algorithm.h"

#define FSETNODE_ARRAY_AMOUNT       4
#define FSETNODE_ARRAY_SIZE         128
#define FSETNODE_MAX_SIZE           (FSETNODE_ARRAY_AMOUNT * FSETNODE_ARRAY_SIZE)
#define FSET_BUCKETS_INIT_SIZE      16

enum EFSetOp {
    EFSetOp_None,
    EFSetOp_Insert,
    EFSetOp_Remove,
};

extern unsigned hash(int key, int hash_pos);

// 哈希表但个值，最小单位
struct FSetValue {
    int value;      // 值
    bool valid;     // 当前值是否有效

    FSetValue(void)
    :value(0),
    valid(false) {}

    FSetValue(int val)
    :value(val),
    valid(true) {}
};

// 对哈希值得操作，具体操作参考EFSetOp
struct FSetOp {
    EFSetOp type;
    FSetValue val;

    FSetOp(void)
    :type(EFSetOp_None)
    {}
};

// 哈希表的一个节点（也可以称作是一个bucket），
// 保存一组hash函数算出来一致的的值
class FSetNode {
public:
    FSetNode(void);
    ~FSetNode(void);

    bool exist(const FSetValue &val);
    bool remove(const FSetValue &val);
    bool insert(const FSetValue &val);
    uint32_t size(void) const;

    // value mod key == des
    // 从本地读取数据到node中
    int split(FSetNode &node, int key, int des);
    // 从本地读取数据到node中
    int merge(FSetNode &node);

    void print(void);
public:
    FSetValue *values_ptr[4];
    FSetOp op;
    uint32_t size_;         // 当前数据的长度
};

// hash集，在 FSetNode 的基础上增加了冻结操作
enum EApplyErr {
    EApplyErr_Ok,    // 成功调用
    EApplyErr_Failed,// 调用失败
    EApplyErr_Freeze,// 当前集合被冻结
};

enum EResizeStatus {
    EResizeStatus_Remain,   // 保持不变
    EResizeStatus_Reduce,   // 缩减
    EResizeStatus_Grow,     // 增长
};

class FSetArray {
public:
    FSetArray(uint32_t size = FSET_BUCKETS_INIT_SIZE);
    ~FSetArray(void);

    // 调用插入和删除
    EApplyErr invoke(const FSetOp &op, FSetArray &pred_set_array);
    // index的桶中OP表示的值是否存在
    bool exist(int index, const FSetOp &op);
    // 数组中一个节点的元素数量
    uint32_t node_elem_size(int index);

    // 冻结当前集合数组
    bool freeze(void);
    // 当前是否冻结
    bool is_freeze(void) {return freeze_;}

    // 桶的的大小
    uint32_t bucket_size(void) {return bucket_size_;}
    // 重置桶的大小
    bool resize(uint32_t size);
    // 检查是否要重置桶的大小
    EResizeStatus when_resize_hash_table(uint32_t ele_size);
    
    // 初始化当前数组中的一个桶
    void init_buckets(int pos, FSetArray &pred_set_array);

    // 打印数组中元素分布
    void print(void);
private:
    FSetNode *buckets_ptr_;
    uint32_t bucket_size_;  // 桶的数量
    bool freeze_; // false：表示没有冻结，true：表示冻结
    uint32_t run_thread_count_;
};
#endif