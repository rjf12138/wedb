#ifndef _DYNAMIC_SIZED__
#define _DYNAMIC_SIZED__
#include "basic/basic_head.h"
#include "system/system.h"

enum FSetOpType {
    FSetOpType_Unknown,
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
    FSetOp(void)
    :type(FSetOpType_Unknown),
    done(false),
    resp(false)
    {}
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
#define DEFAULT_BUCKET_ELEMENTS_SIZE    32
#define DEFAULT_BUCKETS_SIZE 16
#define MIN_ELEMENT_SIZE (DEFAULT_BUCKET_ELEMENTS_SIZE * DEFAULT_BUCKETS_SIZE)

struct HNode {
    std::vector<FSet*> buckets_;
    HNode* pred_ptr;
    uint32_t size;

    HNode(uint32_t s)
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
    :head_ptr_(nullptr) {
        head_ptr_ = new HNode(DEFAULT_BUCKETS_SIZE);
        for (int i = 0; i < DEFAULT_BUCKETS_SIZE; ++i) {
            head_ptr_->buckets_.push_back(new FSet());
        }
        elements_num_ = 0;
        upper_elements_num_ = DEFAULT_BUCKETS_SIZE * DEFAULT_BUCKET_ELEMENTS_SIZE;
        lower_elements_num_ = upper_elements_num_;
    }
    ~LockFreeDSHSet(void) {}

    bool insert(const SValue &k) {
        int pos = hash_function(k, head_ptr_->size);
        if (apply(FSetOpType_Ins, k) == true) {
            ++elements_num_;
            if (elements_num_ > upper_elements_num_) {
                resize(true);
            }
            return true;
        }
        return false;
    }

    bool remove(const SValue &k) {
        int pos = hash_function(k, head_ptr_->size);
        if (apply(FSetOpType_Rem, k) == true) {
            --elements_num_;
            if (elements_num_ < lower_elements_num_) {
                resize(false);
            }
            return true;
        }
        return false;
    }

    bool contains(SValue k) {
        FSet &tset = *(head_ptr_->buckets_[hash_function(k, head_ptr_->buckets_.size())]);
        if (tset.size() == 0) {
            HNode *pred_ptr = head_ptr_->pred_ptr;
            if (pred_ptr != nullptr) {
                tset = *(pred_ptr->buckets_[hash_function(k, pred_ptr->buckets_.size())]);
            }
        }

        return tset.has_member(k);
    }

    void resize(bool grow) {
        HNode *tnode_ptr = head_ptr_;
        if (tnode_ptr->buckets_.size() > DEFAULT_BUCKETS_SIZE || grow == true) {
            for (int i = 0; i < tnode_ptr->buckets_.size(); ++i) {
                init_bucket(i);
            }

            if (head_ptr_->pred_ptr != nullptr) {
                delete head_ptr_->pred_ptr;
                head_ptr_->pred_ptr = nullptr;
            }

            int new_size = grow ? tnode_ptr->buckets_.size() * 2 : tnode_ptr->buckets_.size() / 2;
            HNode* new_node_ptr = new HNode(new_size);
            lower_elements_num_ = upper_elements_num_ / 2;
            upper_elements_num_ = new_size * DEFAULT_BUCKET_ELEMENTS_SIZE;
            if (!os::Mutex::compare_and_swap<HNode*>(head_ptr_, tnode_ptr, new_node_ptr)) {
                delete new_node_ptr;
            }
        }
    }
    
    bool apply(FSetOpType op_type, SValue val) {
        FSetOp op;
        op.key = val;
        op.type = op_type;

        while (true) {
            FSet* set_ptr = head_ptr_->buckets_[hash_function(val, head_ptr_->size)];
            if (set_ptr == nullptr) {
                set_ptr = init_bucket(hash_function(val, head_ptr_->size));
            }

            if (set_ptr != nullptr) {
                set_ptr->invoke(op);
                return set_ptr->get_response(op);
            }
        }

        return false;
    }

    // ∧ 口朝下是 and
    FSet * init_bucket(int pos) {
        if (pos >= head_ptr_->buckets_.size() || pos < 0) {
            return nullptr;    
        }

        FSet *set_ptr = new FSet();
        HNode *pred_ptr = head_ptr_->pred_ptr;
        if (pred_ptr != nullptr) {
            if (head_ptr_->buckets_.size() == pred_ptr->buckets_.size() * 2) {
                FSet &mset = *(pred_ptr->buckets_[pos % pred_ptr->buckets_.size()]);   
                mset.move_value(head_ptr_->buckets_.size(), pos, *set_ptr);
            } else {
                set_ptr->append(*pred_ptr->buckets_[pos]);
                set_ptr->append(*pred_ptr->buckets_[pos + head_ptr_->buckets_.size()]);
            }
            
            bool ret = os::Mutex::compare_and_swap<FSet*>(head_ptr_->buckets_[pos], nullptr, set_ptr);
            if (!ret) {
                delete set_ptr;
                set_ptr = nullptr;
            }
        }
        return set_ptr;
    }

private:
    bool compare_and_swap(void *reg, void *old_value, void *new_value) {
        if (reg != old_value) {
            return false;
        }
        reg = new_value;
        return true;
    }
    
private:
    HNode *head_ptr_;

    uint32_t elements_num_;
    uint32_t lower_elements_num_;
    uint32_t upper_elements_num_;
};
#endif