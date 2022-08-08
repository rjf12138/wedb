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

int hash(int key, int value) {
    return value % key; 
}

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

    // 数组改成动态增长的，当前问题如果增长时复制旧值，但是复制完后进行删除操作了
    // 之后继续进行插入新值，那么已经被删除的值又被恢复了。
    // 或是继续采用固定长度的数组思考
    bool insert(const FSetValue &val) {
        if (size_ == max_size_) {
            FSetValue *tmp_ptr = new FSetValue[max_size_*2];
            for (int i = 0, j = 0; i < max_size_; ++i) {

            }
        }

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
            if (values_ptr[i].valid == true && hash(key, values_ptr[i].value) == des) {
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

class HNode {
public:
    HNode(void)
    {
    }

    ~HNode(void) {
        if (buckets_ptr_ != nullptr) {
            delete buckets_ptr_;
            buckets_ptr_ = nullptr;
        }
    }

    // 添加功能什么时候扩大哈希表，什么时候缩小哈希表

private:
    int resize(bool grow) {
        if (curr_size_ < 2) {
            return curr_size_;
        }

        for (int i = 0; i < curr_size_; ++i) {
            init_buckets(i);
        }

        for (int i = 0; i < pred_size_; ++i) {
            auto tmp_bucket_ptr = pred_buckets_ptr_[i];
            pred_buckets_ptr_[i] = nullptr;
            if (tmp_bucket_ptr != nullptr) {
                delete tmp_bucket_ptr;
            }
        }
        delete[] pred_buckets_ptr_;
        pred_buckets_ptr_ = buckets_ptr_;

        int size = (grow == true ? curr_size_ * 2 : curr_size_ / 2);
        FSet **tmp_buckets_ptr_ = new FSet*[size];
        os::Atomic<FSet**>::compare_and_swap(&buckets_ptr_, buckets_ptr_, tmp_buckets_ptr_);

        return size;
    }

    void init_buckets(int pos) {
        FSet *bucket_ptr = buckets_ptr_[pos];
        if (bucket_ptr == nullptr && pred_buckets_ptr_ != nullptr) {
            FSet *new_bucket_ptr = new FSet();
            if (this->curr_size_ == pred_size_ * 2) {
                FSet *pred_bucket_ptr = pred_buckets_ptr_[hash(pos, pred_size_)];
                if (pred_bucket_ptr != nullptr) {
                    pred_bucket_ptr->freeze();
                    pred_bucket_ptr->node_.split(new_bucket_ptr->node_, pos, bucket_ptr->node_.size());
                }
            } else {
                if (pred_buckets_ptr_[pos] != nullptr) {
                    pred_buckets_ptr_[pos]->freeze();
                    pred_buckets_ptr_[pos]->node_.merge(new_bucket_ptr->node_);
                }

                if (pred_buckets_ptr_[pos + pred_size_] != nullptr) {
                    pred_buckets_ptr_[pos + pred_size_]->freeze();
                    pred_buckets_ptr_[pos + pred_size_]->node_.merge(new_bucket_ptr->node_);
                }
            }
            os::Atomic<FSet*>::compare_and_swap(&buckets_ptr_[pos], nullptr, new_bucket_ptr);
        }
    }

private:
    int curr_size_;
    FSet **buckets_ptr_;        // 当前的hash数据集

    int pred_size_;
    FSet **pred_buckets_ptr_;   // 前置的hash数据集
};

#endif