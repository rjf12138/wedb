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
    EApplyErr_Ok,
    EApplyErr_Failed,
    EApplyErr_Freeze,
};

class FSet {
public:
    FSet(void);
    ~FSet(void);

    void freeze(void);
    EApplyErr invoke(const FSetOp &op);
    bool exist(const FSetOp &op);

public:
    FSetNode node_;
    bool freeze_; // false：表示没有冻结，true：表示冻结
    int index_;
};

class FSetArray {
public:
    FSetArray(uint32_t size = FSET_BUCKETS_INIT_SIZE);
    ~FSetArray(void);

    FSetNode *node(int index);
    FSet* set(int index);
    bool freeze(void);
    bool is_freeze(void) {return freeze_;}
    uint32_t size(void) {return size_;}
    bool resize(uint32_t size);
    
private:
    FSet *buckets_ptr_;
    uint32_t size_;
    bool freeze_; // false：表示没有冻结，true：表示冻结
};
#endif