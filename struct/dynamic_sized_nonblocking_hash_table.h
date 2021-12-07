#ifndef _DYNAMIC_SIZED__
#define _DYNAMIC_SIZED__
#include "basic/basic_head.h"

enum FSetOpType {
    FSetOpType_Ins,
    FSetOpType_Rem,
};

struct SValue {
    bool is_vaild;
    int value;

    SValue(void)
    :is_vaild(false),
    value(0)
    {}
};

int hash_function(SValue key, int seed) 
{
    return key.value % seed;
}

struct FSetOp {
    FSetOpType type;    // 操作类型
    SValue key;            // 关键字
    bool done;          // 操作是否完成
    bool resp;          // 结果返回，如果操作的确进行了设为true,反之false
                        // 比如进行插入操作，值之前已经存在则为false，不存在为true
};

class FSet {
public:
    FSet(void) 
    :ok(true)
    {}
    ~FSet(void){}

    bool get_response(FSetOp op) {return op.resp;}
    bool has_member(SValue key) {
        for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
            if (iter->is_vaild == true && key.value == iter->value) {
                return true;
            }
        }
        return false;
    }

    bool invoke(FSetOp op) {
        if (ok && !op.done) {
            if (op.type == FSetOpType_Ins) {
                op.resp = !has_member(op.key);
                if (op.resp) {
                    data_.insert(op.key);
                }
            } else if (op.type == FSetOpType_Rem) {
                op.resp = has_member(op.key);
                if (op.resp) {
                    data_.erase(op.key);
                }
            }
        }
    }

    void freeze(void) {
        ok = false;
    }

    int size(void) {return data_.size();}
    // 对当前FSet中的元素根据seed和hash_function计算值等于result的移到to中
    int move_value(int seed, int result, FSet &to) {
        int size = 0;
        for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
            if (hash_function(*iter, seed) == result) {
                to.data_.insert(*iter);
                data_.erase(iter);
                ++size;
            }
        }

        return size;
    }

    void append(FSet &from) {
        data_.insert(from.data_.begin(), from.data_.end());
        from.data_.clear();
    }
private:
    std::set<SValue> data_;
    bool ok;
};

//////////////////////////////////////////////////////////
struct HNode {
    std::vector<FSet*> buckets_;
    HNode* pred_ptr;
    int size;

    HNode(int s)
    :pred_ptr(nullptr),
     size(s)
    {
        buckets_.assign(size, nullptr);   
    }

    ~HNode(void) {
        for (auto iter = buckets_.begin(); iter != buckets_.end(); ++iter) {
            delete *iter;
            *iter = nullptr;
        }
    }
};

class LockFreeDSHSet {
public:
    LockFreeDSHSet(void) 
    :head_ptr(nullptr) {
        head_ptr = new HNode(1);
        head_ptr->buckets_.push_back(new FSet());
    }
    ~LockFreeDSHSet(void) {}

    bool insert(int k) {

    }

    bool remove(int k) {

    }

    bool contains(SValue k) {
        FSet &tset = *(head_ptr->buckets_[hash_function(k, head_ptr->buckets_.size())]);
        if (tset.size() == 0) {
            HNode *pred_ptr = head_ptr->pred_ptr;
            if (pred_ptr != nullptr) {
                tset = *(pred_ptr->buckets_[hash_function(k, pred_ptr->buckets_.size())]);
            }
        }

        return tset.has_member(k);
    }

    void resize(bool grow) {
        if (head_ptr->buckets_.size() > 1 && grow == true) {

        }
    }
    
    // ∧ 口朝下是 and
    int init_bucket(int pos) {
        if (pos >= head_ptr->buckets_.size() || pos < 0) {
            return -1;    
        }

        FSet *set_ptr = new FSet();
        HNode *pred_ptr = head_ptr->pred_ptr;
        if (pred_ptr != nullptr) {
            if (head_ptr->buckets_.size() == pred_ptr->buckets_.size() * 2) {
                FSet &mset = *(pred_ptr->buckets_[pos % pred_ptr->buckets_.size()]);   
                mset.move_value(head_ptr->buckets_.size(), pos, *set_ptr);
            } else {
                set_ptr->append(*pred_ptr->buckets_[pos]);
                set_ptr->append(*pred_ptr->buckets_[pos + head_ptr->buckets_.size()]);
            }
            // 研究一下CAS
            std::atomic_compare_exchange_strong(head_ptr->buckets_[pos], nullptr, set_ptr);
        }
    }

private:
    int growing_policy(void);
    int shinking_policy(void);
    
private:
    HNode *head_ptr;
};
#endif