#ifndef _DYNAMIC_SIZED__
#define _DYNAMIC_SIZED__

#include "basic/basic_head.h"
#include "system/system.h"
#include "doubly_list.h"

enum FSetOpType {
    FSetOpType_Unknown,
    FSetOpType_Ins,
    FSetOpType_Rem,
};

struct SValue {
    bool is_vaild;
    int value;

    bool operator==(const SValue& rhs) const {
        return value == rhs.value;
    }
    bool operator!=(const SValue& rhs) const {
        return !(value == rhs.value);
    }
    bool operator>(const SValue& rhs) const {
        return (value > rhs.value);
    }
    bool operator>=(const SValue& rhs) const {
        return (value >= rhs.value);
    }
    bool operator<(const SValue& rhs) const {
        return (value < rhs.value);
    }
    bool operator<=(const SValue& rhs) const {
        return (value <= rhs.value);
    }
    SValue& operator=(const SValue& src) {
        is_vaild = src.is_vaild;
        value = src.value;

        return *this;
    }

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
    SValue key;         // 关键字
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
    :ok(true) {}
    ~FSet(void){}

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
                    op.key.is_vaild = true;
                    data_.push_back(op.key);
                    return true;
                }
            } else if (op.type == FSetOpType_Rem) {
                 for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
                    if (iter->is_vaild == true && op.key.value == iter->value) {
                        data_.erase(iter);
                        return true;
                    }
                }
            }
        }
        return false;
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
                to.data_.push_back(*iter);
                data_.erase(iter);
                ++size;
            }
        }

        return size;
    }

    void append(FSet &from) {
        DoublyList<SValue>::Iterator iter = from.data_.begin();
        for (; iter != from.data_.end(); ++iter) {
            data_.push_back(*iter);
        }
        from.data_.clear();
    }
public:
    DoublyList<SValue> data_;
    bool ok;
};

//////////////////////////////////////////////////////////
#define DEFAULT_BUCKET_ELEMENTS_SIZE    32
#define DEFAULT_BUCKETS_SIZE 16
#define MIN_ELEMENT_SIZE (DEFAULT_BUCKET_ELEMENTS_SIZE * DEFAULT_BUCKETS_SIZE)

struct HNode {
    FSet **buckets_;
    HNode* pred_ptr;
    uint32_t size;

    HNode(uint32_t s)
    :pred_ptr(nullptr),
     size(s) {
        buckets_ = new FSet*[size];
        for (uint32_t i = 0; i < size; ++i) {
            buckets_[size] = nullptr;
        }
    }

    ~HNode(void) {
        for (uint32_t i = 0; i < size; ++i) {
            if (buckets_[i] != nullptr) {
                delete buckets_[i];
            }
        }
        delete []buckets_;
    }
};

class LockFreeDSHSet {
public:
    LockFreeDSHSet(void) 
    :head_ptr_(nullptr) {
        head_ptr_ = new HNode(DEFAULT_BUCKETS_SIZE);
        elements_num_ = 0;
        upper_elements_num_ = DEFAULT_BUCKETS_SIZE * DEFAULT_BUCKET_ELEMENTS_SIZE;
        lower_elements_num_ = upper_elements_num_;
    }
    ~LockFreeDSHSet(void) {
        if (head_ptr_ != nullptr) {
            if (head_ptr_->pred_ptr != nullptr) {
                delete head_ptr_->pred_ptr;
                head_ptr_->pred_ptr = nullptr;
            }
            delete head_ptr_;
            head_ptr_ = nullptr;
        }

    }

    int size(void) const {return elements_num_;}

    bool insert(const SValue &k) {
        bool ret = apply(FSetOpType_Ins, k);
        if (ret == true) {
            ++elements_num_;
            if (elements_num_ > upper_elements_num_) {
                resize(true);
            }
            return true;
        }
        return false;
    }

    bool remove(const SValue &k) {
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
        FSet *tset = head_ptr_->buckets_[hash_function(k, head_ptr_->size)];
        if (tset == nullptr) {
            HNode *pred_ptr = head_ptr_->pred_ptr;
            if (pred_ptr != nullptr) {
                tset = pred_ptr->buckets_[hash_function(k, pred_ptr->size)];
            }
        }

        if (tset != nullptr) {
            return tset->has_member(k);
        }
        return false;
    }

    void resize(bool grow) {
        HNode *tnode_ptr = head_ptr_;
        if (tnode_ptr->size > DEFAULT_BUCKETS_SIZE || grow == true) {
            for (int i = 0; i < tnode_ptr->size; ++i) {
                init_bucket(i);
            }

            if (head_ptr_->pred_ptr != nullptr) {
                delete head_ptr_->pred_ptr;
                head_ptr_->pred_ptr = nullptr;
            }

            int new_size = grow ? tnode_ptr->size * 2 : tnode_ptr->size / 2;
            HNode* new_node_ptr = new HNode(new_size);
            lower_elements_num_ = upper_elements_num_ / 2;
            upper_elements_num_ = new_size * DEFAULT_BUCKET_ELEMENTS_SIZE;
            if (!os::Atomic<HNode*>::compare_and_swap(&new_node_ptr->pred_ptr, nullptr, head_ptr_)) {
                delete new_node_ptr;
            } else {
                head_ptr_ = new_node_ptr;
            }
        }
    }
    
    bool apply(FSetOpType op_type, SValue val) {
        FSetOp op;
        op.key = val;
        op.type = op_type;

        while (true) {
            FSet **set_ptr = &head_ptr_->buckets_[hash_function(val, head_ptr_->size)];
            if (*set_ptr == nullptr) {
                *set_ptr = init_bucket(hash_function(val, head_ptr_->size));
            }

            if (*set_ptr != nullptr) {
                return (*set_ptr)->invoke(op);
            }
        }

        return false;
    }
    
    // ∧ 口朝下是 and
    FSet* init_bucket(int pos) {
        if (pos >= head_ptr_->size || pos < 0) {
            return nullptr;    
        }

        FSet *set_ptr = new FSet();
        HNode *pred_ptr = head_ptr_->pred_ptr;
        if (pred_ptr != nullptr) {
            if (head_ptr_->size == pred_ptr->size * 2) {
                FSet *mset = pred_ptr->buckets_[pos % pred_ptr->size];   
                mset->move_value(head_ptr_->size, pos, *set_ptr);
            } else {
                set_ptr->append(*(pred_ptr->buckets_[pos]));
                set_ptr->append(*(pred_ptr->buckets_[pos + head_ptr_->size]));
            }
            
            bool ret = os::Atomic<FSet*>::compare_and_swap(&head_ptr_->buckets_[pos], nullptr, set_ptr);
            if (ret == false) {
                delete set_ptr;
            }
        }
        return set_ptr;
    }

    void print(void) {
        if (head_ptr_ == nullptr) {
            fprintf(stderr, "Empty Hash table!\n");
            return ;
        }

        fprintf(stderr, "Head_ptr: \n");
        for (int i = 0; i < head_ptr_->size; ++i) {
            fprintf(stderr, "  buckets[%d]: ", i);
            if (head_ptr_->buckets_[i] != nullptr) {
                for (auto iter = head_ptr_->buckets_[i]->data_.begin(); iter != head_ptr_->buckets_[i]->data_.end(); ++iter) {
                    fprintf(stderr, "%d ", iter->value);
                }
            } else {
                fprintf(stderr, "nullptr");
            }
            fprintf(stderr, "\n");
        }

        fprintf(stderr, "Pred_ptr: \n");
        if (head_ptr_->pred_ptr == nullptr) {
            fprintf(stderr, "nullptr\n");
        } else {
            HNode *tmp_ptr = head_ptr_->pred_ptr;
            for (int i = 0; i < tmp_ptr->size; ++i) {
            fprintf(stderr, "  buckets[%d]: ", i);
            if (tmp_ptr->buckets_[i] != nullptr) {
                for (auto iter = tmp_ptr->buckets_[i]->data_.begin(); iter != tmp_ptr->buckets_[i]->data_.end(); ++iter) {
                    fprintf(stderr, "%d ", iter->value);
                }
            } else {
                fprintf(stderr, "nullptr");
            }
            fprintf(stderr, "\n");
        }
        }

        fprintf(stderr, "\n\n");
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