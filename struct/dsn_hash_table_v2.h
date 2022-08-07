#ifndef _DYNAMIC_SIZED_V2__
#define _DYNAMIC_SIZED_V2__

#include "basic/basic_head.h"
#include "system/system.h"

#define FSETNODE_VALUE_MAX_SIZE     1024
#define FSET_BUCKETS_INIT_SIZE      16

enum EFSetOp {
    EFSetOp_None,
    EFSetOp_WaitFree,
    EFSetOp_Insert,
    EFSetOp_Remove,
};

struct FSetValue {
    int value;      // 值
    bool valid;     // 当前值是否有效

    int hash(int key) {
        return value % key; 
    }

    FSetValue(void)
    :value(0),
    valid(false) {}

    FSetValue(int val)
    :value(val),
    valid(true) {}
};

struct FSetOp {
    EFSetOp type;
    FSetValue val;
    bool resp;
    int prio;

    FSetOp(void)
    :type(EFSetOp_None),
    resp(false),
    prio(0)
    {}
};

enum EFSetOperate {
    EFSetOperate_Merge,         // 合并
    EFSetOperate_FirstSplit,    // 第一次分割
    EFSetOperate_SecondSplit,   // 将剩余的元素分割
};

struct FSetNode {
    FSetNode(uint32_t size = FSETNODE_VALUE_MAX_SIZE)
    :size_(0),
    max_size_(size) {
        values_ptr = new FSetValue[size];
    }

    ~FSetNode(void) {
        if (values_ptr != nullptr) {
            delete []values_ptr;
            size_ = 0;
        }
    }

    void clear(void) {
        
    }

    bool exist(const FSetValue &val) {
        for (int i = 0; i < max_size_; ++i) {
            if (values_ptr[i].valid == true && val.value == values_ptr[i].value) {
                return true;
            }
        }

        return false;
    }

    bool remove(const FSetValue &val) {
        for (int i = 0; i < max_size_; ++i) {
            if (values_ptr[i].valid == true && val.value == values_ptr[i].value) {
                if (os::Atomic<bool>::compare_and_swap(&values_ptr[i].valid, true, false)) {
                    os::Atomic<int>::fetch_and_sub(&size_, 1);
                    return true;
                }
            }
        }
        return false;
    }

    bool insert(const FSetValue &val) {
        for (int i = 0; i < max_size_; ++i) {
            if (os::Atomic<bool>::compare_and_swap(&values_ptr[i].valid, false, true)) {
                values_ptr[i].value = val.value;
                os::Atomic<int>::fetch_and_add(&size_, 1);
                return true;
            }
        }

        return false;
    }
    uint32_t size(void) const {return size_;}

    // value mod key == des
    // 从本地读取数据到node中
    int split(FSetNode &node, int key, int des) {
        int move_size = 0;
        for (int i = 0; i < max_size_; ++i) {
            if (values_ptr[i].valid == true && values_ptr[i].hash(key) == des) {
                if (os::Atomic<bool>::compare_and_swap(&values_ptr[i].valid, true, false) && node.insert(values_ptr[i]) == true) {
                    os::Atomic<int>::fetch_and_sub(&size_, 1);
                    ++move_size;
                } else {
                    break;
                }
            }
        }
        return move_size;
    }

    // 从本地读取数据到node中
    int merge(FSetNode &node) {
        int move_size = 0;
        for (int i = 0; i < max_size_; ++i) {
            if (values_ptr[i].valid == true) {
                if (os::Atomic<bool>::compare_and_swap(&values_ptr[i].valid, true, false) && node.insert(values_ptr[i])) {
                    os::Atomic<int>::fetch_and_sub(&size_, 1);
                    ++move_size;
                } else {
                    break;
                }
            }
        }
        return move_size;
    }
public:
    FSetValue *values_ptr;
    FSetOp op;
    uint32_t size_;
    uint32_t max_size_;
};

class FSet {
public:
    FSet(void) 
    :flag_(false) {}

    bool invoke(FSetOp op) {
        while (node_.op.type != EFSetOp_WaitFree && node_.op.prio != -1) {
            if (flag_ == true) {
                do_freeze();
                break;
            }

            FSetOp tmp_op = node_.op;
            if (node_.op.type == EFSetOp_None) {
                if (op.prio != -1) {
                    if (os::Atomic<FSetOp>::compare_and_swap(&node_.op, tmp_op, op)) {
                        help_finish();
                        return true;
                    }
                }
            } else {
                help_finish();
            }
        }
        return op.prio == -1;
    }

    void freeze(void) {
        flag_ = true;
        return do_freeze();
    }

    void do_freeze(void) {
        while (node_.op.type != EFSetOp_WaitFree) {
            FSetOp &tmp = node_.op;
            if (tmp.type == EFSetOp_None) {
                if (os::Atomic<EFSetOp>::compare_and_swap(&tmp.type, EFSetOp_None, EFSetOp_WaitFree)) {
                    break;
                }
            } else {
                help_finish();
                break;
            }
        }
    }

    bool help_finish(void) {
        FSetOp &op = node_.op;
        op.resp = false;
        if (op.type != EFSetOp_None && op.type != EFSetOp_WaitFree) {
            bool is_exist = node_.exist(op.val);
            if (op.type == EFSetOp_Insert) {
                if (is_exist == false && node_.insert(op.val) == true) {
                    op.resp = true;
                }
            } else {
                if (is_exist == true && node_.remove(op.val) == true) {
                    op.resp = true;
                }
            }
        }
        op.prio = -1; // -1表示∞
        return op.resp;
    }

    bool has_member(FSetValue val) {
        FSetOp &op = node_.op;
        if (op.type != EFSetOp_None && op.type != EFSetOp_WaitFree && op.val.value == val.value) {
            return op.type == EFSetOp_Insert;
        }
        return node_.exist(val);
    }

public:
    FSetNode node_;
    bool flag_;
};
//  TODO
// HNode 初始化问题 init_bucket会初始化HNode中的某个bucket
// 但现在两边HNode和DSSet不大相容
class HNode {
public:
    HNode(void):
    size_(0),
    buckets_ptr_(nullptr),
    pred_ptr_(nullptr) 
    {
        resize(16);
    }

    ~HNode(void) {
        if (buckets_ptr_ != nullptr) {
            delete buckets_ptr_;
            buckets_ptr_ = nullptr;
        }
    }

    int resize(int size) {
        if (size < size_ || size < 0) {
            return -1;
        }

        if (buckets_ptr_ != nullptr) {
            delete buckets_ptr_;
            buckets_ptr_ = nullptr;
        }
        buckets_ptr_ = new FSet*[size];
        size_ = size;

        return size;
    }

    // 从node中读取数据到本地
    int split(FSet &node, int pos, int key, int des) {
        node.freeze();
        return node.node_.split(buckets_ptr_[pos]->node_, size_, pos);
    }

    // 从node中读取数据到本地
    int merge(FSet &node, int pos) {
        node.freeze();
        return node.node_.merge(buckets_ptr_[pos]->node_);
    }

public:
    int size_;
    FSet **buckets_ptr_;
    HNode *pred_ptr_;
};

class DSHSet {
public:
    DSHSet(void);
    ~DSHSet(void);

    bool contain(FSetValue val) {
        FSet *set = node_.buckets_ptr_[val.hash(node_.size_)];
        if (set->has_member(val) == false) {
            if (node_.pred_ptr_ != nullptr) {
                set = node_.pred_ptr_->buckets_ptr_[val.hash(node_.size_)];
            } 
        }

        return set->has_member(val);
    }

    int resize(bool grow) {
        if (node_.size_ > 1 && grow == true) {

        }
    }

    void init_buckets(int pos) {
        FSet *set_ptr = node_.buckets_ptr_[pos];
        HNode *pred_ptr = node_.pred_ptr_;
        if (set_ptr->node_.size() == 0 && pred_ptr != nullptr) {
            FSet *new_bucket_ptr = new FSet();
            if (node_.size_ == pred_ptr->size_ * 2) {
                node_.split();
            } else {
                pred_ptr->buckets_ptr_[pos].freeze();
                pred_ptr->buckets_ptr_[pos].node_.merge(new_bucket_ptr->node_);

                pred_ptr->buckets_ptr_[pos + pred_ptr->size_].freeze();
                pred_ptr->buckets_ptr_[pos + pred_ptr->size_].node_.merge(new_bucket_ptr->node_);
            }
            os::Atomic<FSet*>::compare_and_swap((node_.buckets_ptr_[pos]), nullptr, new_bucket_ptr);
        }
    }
private:
    HNode node_;
};

#endif