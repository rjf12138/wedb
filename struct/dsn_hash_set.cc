#include "dsn_hash_set.h"

int hash(int key) 
{
    int ret = 0;
    algorithm::murmurhash3_x86_32(&key, sizeof(key), 12138, &ret);
    return ret;
}

////////////////////////////////////////////////////////////////////////////
FSetNode::FSetNode(void)
:size_(0)
{
    for (int i = 0; i < 4; ++i) {
        values_ptr[i] = nullptr;
    }
    values_ptr[0] = new FSetValue[FSETNODE_VALUE_MAX_SIZE];
}

FSetNode::~FSetNode(void) 
{
    for (int i = 0; i < 4; ++i) {
        if (values_ptr[i] != nullptr) {
            delete []values_ptr[i];
        }
    }
}

bool 
FSetNode::exist(const FSetValue &val) 
{
    for (int j = 0; j < 4; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }

        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values_ptr[j][i].valid == true && val.value == values_ptr[j][i].value) {
                return true;
            }
        }
    }

    return false;
}

bool 
FSetNode::remove(const FSetValue &val) 
{
    for (int j = 0; j < 4; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }

        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values_ptr[j][i].valid == true && val.value == values_ptr[j][i].value) {
                if (os::Atomic<bool>::compare_and_swap(&values_ptr[j][i].valid, true, false)) {
                    os::Atomic<int>::fetch_and_sub(&size_, 1);
                    return true;
                }
            }
        }
    }
    return false;
}

bool 
FSetNode::insert(const FSetValue &val) 
{
    for (int j = 0; j < 4; ++j) {
        if (values_ptr[j] == nullptr) {
            auto tmp_ptr = new FSetValue[FSETNODE_VALUE_MAX_SIZE];
            bool ret = os::Atomic<FSetValue*>::compare_and_swap(&values_ptr[j], nullptr, tmp_ptr);
            if (ret == false) {
                delete[] tmp_ptr;
            }
        }

        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (os::Atomic<bool>::compare_and_swap(&values_ptr[j][i].valid, false, true)) {
                values_ptr[j][i].value = val.value;
                os::Atomic<int>::fetch_and_add(&size_, 1);
                return true;
            }
        }
    }

    return false;
}
uint32_t 
FSetNode::size(void) const 
{
    return size_;
}

// value mod key == des
// 从本地读取数据到node中
int 
FSetNode::split(FSetNode &node, int key, int des) 
{
    int move_size = 0;

    for (int j = 0; j < 4; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }

        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values_ptr[j][i].valid == true && hash(key) == des) {
                if (os::Atomic<bool>::compare_and_swap(&values_ptr[j][i].valid, true, false) && node.insert(values_ptr[j][i]) == true) {
                    os::Atomic<int>::fetch_and_sub(&size_, 1);
                    ++move_size;
                } else {
                    break;
                }
            }
        }
    }
    return move_size;
}

// 从本地读取数据到node中
int 
FSetNode::merge(FSetNode &node) 
{
    int move_size = 0;
    for (int j = 0; j < 4; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }
        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values_ptr[j][i].valid == true) {
                if (os::Atomic<bool>::compare_and_swap(&values_ptr[j][i].valid, true, false) && node.insert(values_ptr[j][i])) {
                    os::Atomic<int>::fetch_and_sub(&size_, 1);
                    ++move_size;
                } else {
                    break;
                }
            }
        }
    }
    return move_size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSet::FSet(void) 
    :freeze_(false) 
{
}

FSet::~FSet(void) {}

void 
FSet::freeze(void) 
{
    while (freeze_ == false) {
        if (os::Atomic<bool>::compare_and_swap(&freeze_, false, true) == true) {
            break;
        }
    }
}

bool 
FSet::invoke(const FSetOp &op) 
{
    while (freeze_ == false) {
        bool ret = false;
        if (op.type == EFSetOp_Insert && node_.exist(op.val) == false) {
            ret = node_.insert(op.val);
        } else if (op.type == EFSetOp_Remove && node_.exist(op.val) == true) {
            ret = node_.remove(op.val);
        }
        return ret;
    }
    return false;
}

bool 
FSet::exist(const FSetOp &op) 
{
    return node_.exist(op.val);
}

/////////////////////////////////////////////////////////////////////////////////////////
DSHashSet::DSHashSet(void)
:max_data_size_(0)
{
}

DSHashSet::~DSHashSet(void) 
{
    if (buckets_ptr_ != nullptr) {
        delete buckets_ptr_;
        buckets_ptr_ = nullptr;
    }
}

bool 
DSHashSet::insert(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    op.type = EFSetOp_Insert;

    bool ret = apply(op);
    if (ret == true) {
        when_resize_hash_table();
    }

    return ret;
}

bool 
DSHashSet::remove(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    op.type = EFSetOp_Remove;

    bool ret = apply(op);
    if (ret == true) {
        when_resize_hash_table();
    }

    return ret;
}

bool 
DSHashSet::exist(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    for (int i = 0; i < curr_size_; ++i) {
        if (buckets_ptr_[i]->exist(op) == true) {
            return true;
        }
    }

    for (int i = 0; i < pred_size_; ++i) {
        if (pred_buckets_ptr_[i]->exist(op) == true) {
            return true;
        }
    }

    return false;
}

int 
DSHashSet::when_resize_hash_table(void) 
{
    unsigned total_size = 0;
    for (int i = 0; i < curr_size_; ++i) {
        if (buckets_ptr_[i] != nullptr) {
            total_size += buckets_ptr_[i]->node_.size();
            // 当莫个桶中元素达到最大值时进行扩大
            if (buckets_ptr_[i]->node_.size() ==  4 * FSETNODE_VALUE_MAX_SIZE) {
                return resize(true);
            }
        }
    }

    for (int i = 0; i < pred_size_; ++i) {
        if (pred_buckets_ptr_[i] != nullptr) {
            total_size += pred_buckets_ptr_[i]->node_.size();
        }
    }

    // 当数据最大容量的超过3分之二时进行扩大
    if (total_size > 0.6 * max_data_size_) {
        return resize(true);
    }

    // 当数据的容量小于桶的数量时进行缩减
    if (total_size < curr_size_) {
        return resize(false);
    }

    return 0;
}

bool 
DSHashSet::apply(FSetOp op) 
{
    while (true) {
        int pos = hash(op.val.value);
        if (buckets_ptr_[pos] == nullptr) {
            init_buckets(pos);
        }

        if (buckets_ptr_[pos]->invoke(op) == true) {
            return true;
        }
    }
    return false;
}

int 
DSHashSet::resize(bool grow) 
{
    if (curr_size_ <= 16) { // 最小维持16个桶
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

    // 计算当前哈希表存储数据的上限
    max_data_size_ = size * FSETNODE_VALUE_MAX_SIZE;

    return size;
}

void 
DSHashSet::init_buckets(int pos) 
{
    FSet *bucket_ptr = buckets_ptr_[pos];
    if (bucket_ptr == nullptr && pred_buckets_ptr_ != nullptr) {
        FSet *new_bucket_ptr = new FSet();
        if (this->curr_size_ == pred_size_ * 2) {
            FSet *pred_bucket_ptr = pred_buckets_ptr_[hash(pos)];
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