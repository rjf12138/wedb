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

template <class T>
struct SValue {
    bool is_vaild;
    T value;

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

    int hash_function(int seed) {
        return value % seed;
    }

    SValue(void)
    :is_vaild(false),
    value(0)
    {}
};

template <class T>
class FSet {
public:
    FSet(void) 
    :ok(true) {}
    ~FSet(void){}

    bool has_member(SValue<T> key) {
        for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
            if (iter->is_vaild == true && key.value == iter->value) {
                return true;
            }
        }
        return false;
    }

    bool invoke(FSetOpType op, SValue<T> &key) {
        if (ok == true) {
            if (op == FSetOpType_Ins) {
                if (!has_member(key)) {
                    key.is_vaild = true;
                    data_.push_back(key);
                    return true;
                }
            } else if (op == FSetOpType_Rem) {
                 for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
                    if (iter->is_vaild == true && key.value == iter->value) {
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
    int move_value(int seed, int result, FSet<T> &to) {
        int size = 0;
        for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
            if (iter->hash_function(seed) == result) {
                to.data_.push_back(*iter);
                ++size;
            }
        }

        return size;
    }

    void append(FSet<T> &from) {
        typename DoublyList<SValue<T>>::Iterator iter = from.data_.begin();
        for (; iter != from.data_.end(); ++iter) {
            data_.push_back(*iter);
        }
        from.data_.clear();
    }
public:
    DoublyList<SValue<T>> data_;
    bool ok;
};

//////////////////////////////////////////////////////////
#define DEFAULT_BUCKET_ELEMENTS_SIZE    32
#define DEFAULT_BUCKETS_SIZE 16
#define MIN_ELEMENT_SIZE (DEFAULT_BUCKET_ELEMENTS_SIZE * DEFAULT_BUCKETS_SIZE)

template <class T>
struct HNode {
    FSet<T> **buckets_;
    HNode<T>* pred_ptr;
    int size;

    HNode(int s)
    :pred_ptr(nullptr),
     size(s) {
        buckets_ = new FSet<T>*[size];
        for (int i = 0; i < size; ++i) {
            buckets_[i] = nullptr;
        }
    }

    ~HNode(void) {
        for (int i = 0; i < size; ++i) {
            if (buckets_[i] != nullptr) {
                delete buckets_[i];
            }
        }
        delete []buckets_;
    }
};

template <class T>
class LockFreeDSHSet {
public:
    LockFreeDSHSet(void) 
    :head_ptr_(nullptr) {
        head_ptr_ = new HNode<T>(DEFAULT_BUCKETS_SIZE);
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

    bool insert(SValue<T> &k) {
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

    bool remove(SValue<T> &k) {
        if (apply(FSetOpType_Rem, k) == true) {
            --elements_num_;
            if (elements_num_ < lower_elements_num_) {
                resize(false);
            }
            return true;
        }
        return false;
    }

    bool contains(SValue<T> k) {
        FSet<T> *tset = head_ptr_->buckets_[k.hash_function(head_ptr_->size)];
        if (tset == nullptr) {
            HNode<T> *pred_ptr = head_ptr_->pred_ptr;
            if (pred_ptr != nullptr) {
                tset = pred_ptr->buckets_[k.hash_function(pred_ptr->size)];
            }
        }

        if (tset != nullptr) {
            return tset->has_member(k);
        }
        return false;
    }

    void resize(bool grow) {
        if (head_ptr_->size > DEFAULT_BUCKETS_SIZE || grow == true) {
            if (head_ptr_->pred_ptr != nullptr) {
                delete head_ptr_->pred_ptr;
                head_ptr_->pred_ptr = nullptr;
            }

            int new_size = grow ? head_ptr_->size * 2 : head_ptr_->size / 2;
            HNode<T>* new_node_ptr = new HNode<T>(new_size);
            lower_elements_num_ = upper_elements_num_ / 2;
            upper_elements_num_ = new_size * DEFAULT_BUCKET_ELEMENTS_SIZE;
            if (!os::Atomic<HNode<T>*>::compare_and_swap(&new_node_ptr->pred_ptr, nullptr, head_ptr_)) {
                delete new_node_ptr;
                return ;
            } else {
                head_ptr_ = new_node_ptr;
            }

            for (int i = 0; i < head_ptr_->size; ++i) {
                FSet<T> *ret_ptr = init_bucket(i);
                if (ret_ptr == nullptr) {
                    head_ptr_->buckets_[i] = ret_ptr;
                }
            }
        }
    }
    
    bool apply(FSetOpType op_type, SValue<T> &val) {
        while (true) {
            FSet<T> **set_ptr = &head_ptr_->buckets_[val.hash_function(head_ptr_->size)];
            if (*set_ptr == nullptr) {
                *set_ptr = init_bucket(val.hash_function(head_ptr_->size));
            }

            if (*set_ptr != nullptr) {
                return (*set_ptr)->invoke(op_type, val);
            }
        }

        return false;
    }
    
    // ∧ 口朝下是 and
    FSet<T>* init_bucket(int pos) {
        if (pos >= head_ptr_->size || pos < 0) {
            return nullptr;    
        }

        FSet<T> *set_ptr = new FSet<T>();
        HNode<T> *pred_ptr = head_ptr_->pred_ptr;
        if (pred_ptr != nullptr) {
            if (head_ptr_->size == pred_ptr->size * 2) {
                FSet<T> *mset = pred_ptr->buckets_[pos % pred_ptr->size];   
                mset->move_value(head_ptr_->size, pos, *set_ptr);
                mset->freeze();
            } else {
                set_ptr->append(*(pred_ptr->buckets_[pos]));
                set_ptr->append(*(pred_ptr->buckets_[pos + head_ptr_->size]));
                pred_ptr->buckets_[pos]->freeze();
                pred_ptr->buckets_[pos + head_ptr_->size]->freeze();
            }
            
            bool ret = os::Atomic<FSet<T>*>::compare_and_swap(&head_ptr_->buckets_[pos], nullptr, set_ptr);
            if (ret == false) {
                delete set_ptr;
                return nullptr;
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
            HNode<T> *tmp_ptr = head_ptr_->pred_ptr;
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
    HNode<T> *head_ptr_;

    uint32_t elements_num_;
    uint32_t lower_elements_num_;
    uint32_t upper_elements_num_;

    std::map<os::thread_id_t, >
};
#endif