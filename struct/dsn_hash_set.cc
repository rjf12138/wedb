#include "dsn_hash_set.h"

unsigned hash(int key, int hash_pos) 
{
    unsigned ret = 0;
    algorithm::murmurhash3_x86_32(&key, sizeof(key), 12138, &ret);
    return ret % hash_pos;
}

////////////////////////////////////////////////////////////////////////////
FSetNode::FSetNode(void)
:size_(0)
{
    for (int i = 0; i < FSETNODE_ARRAY_AMOUNT; ++i) {
        values_ptr[i] = nullptr;
    }
    values_ptr[0] = new FSetValue[FSETNODE_ARRAY_SIZE];
}

FSetNode::~FSetNode(void) 
{
    for (int i = 0; i < FSETNODE_ARRAY_AMOUNT; ++i) {
        if (values_ptr[i] != nullptr) {
            delete []values_ptr[i];
        }
    }
}

bool 
FSetNode::exist(const FSetValue &val) 
{
    for (int j = 0; j < FSETNODE_ARRAY_AMOUNT; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }

        for (int i = 0; i < FSETNODE_ARRAY_SIZE; ++i) {
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
    for (int j = 0; j < FSETNODE_ARRAY_AMOUNT; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }

        for (int i = 0; i < FSETNODE_ARRAY_SIZE; ++i) {
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
    for (int j = 0; j < FSETNODE_ARRAY_AMOUNT; ++j) {
        if (values_ptr[j] == nullptr) {
            auto tmp_ptr = new FSetValue[FSETNODE_ARRAY_SIZE];
            bool ret = os::Atomic<FSetValue*>::compare_and_swap(&values_ptr[j], nullptr, tmp_ptr);
            if (ret == false) {
                delete[] tmp_ptr;
            }
        }

        for (int i = 0; i < FSETNODE_ARRAY_SIZE; ++i) {
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
    for (int j = 0; j < FSETNODE_ARRAY_AMOUNT; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }

        for (int i = 0; i < FSETNODE_ARRAY_SIZE; ++i) {
            if (values_ptr[j][i].valid == true && hash(values_ptr[j][i].value, des) == key) {
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
    for (int j = 0; j < FSETNODE_ARRAY_AMOUNT; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }
        for (int i = 0; i < FSETNODE_ARRAY_SIZE; ++i) {
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

void 
FSetNode::print(void)
{
    int cnt = 0;
    for (int j = 0; j < FSETNODE_ARRAY_AMOUNT; ++j) {
        if (values_ptr[j] == nullptr) {
            continue;
        }
        for (int i = 0; i < FSETNODE_ARRAY_SIZE; ++i) {
            if (values_ptr[j][i].valid == true) {
                fprintf(stdout, "%d ", values_ptr[j][i].value);
                ++cnt;
                if (cnt % 15 == 0) {
                    fprintf(stdout, "\n");
                }
            }
        }
    }
    fprintf(stdout, "\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int i = 0;
FSet::FSet(void) 
    :freeze_(false) 
{
    index_ = ++i;
    LOG_GLOBAL_INFO("Create %d", index_);
}

FSet::~FSet(void) 
{
    LOG_GLOBAL_INFO("Destroy %d", index_);
}

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
:
curr_size_(0),
buckets_ptr_(nullptr),
pred_size_(0),
pred_buckets_ptr_(nullptr)
{
    resize(false);
}

DSHashSet::~DSHashSet(void) 
{
    if (buckets_ptr_ != nullptr) {
        for (int i = 0; i < curr_size_; ++i) {
            if (buckets_ptr_[i] != nullptr) {
                delete buckets_ptr_[i];
            }
        }
        delete []buckets_ptr_;
        buckets_ptr_ = nullptr;
    }

    if (pred_buckets_ptr_ != nullptr) {
        for (int i = 0; i < pred_size_; ++i) {
            if (pred_buckets_ptr_[i] != nullptr) {
                delete pred_buckets_ptr_[i];
            }
        }
        delete [] buckets_ptr_;
        pred_buckets_ptr_ = nullptr;
    }
}

bool 
DSHashSet::insert(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    op.type = EFSetOp_Insert;

    return apply(op);
}

bool 
DSHashSet::remove(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    op.type = EFSetOp_Remove;

    return apply(op);
}

bool 
DSHashSet::exist(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    int pos = hash(key, this->curr_size_);
    if (buckets_ptr_[pos] != nullptr && buckets_ptr_[pos]->exist(op) == true) {
        return true;
    }

    if (this->pred_size_ > 0) {
        pos = hash(key, this->pred_size_);
        if (pred_buckets_ptr_[pos] != nullptr && pred_buckets_ptr_[pos]->exist(op) == true) {
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
            // 当某个桶中元素超过四分之三时进行扩大
            if (buckets_ptr_[i]->node_.size() > FSETNODE_MAX_SIZE * 0.8) {
                return resize(true);
            }
        }
    }

    for (int i = 0; i < pred_size_; ++i) {
        if (pred_buckets_ptr_[i] != nullptr) {
            total_size += pred_buckets_ptr_[i]->node_.size();
        }
    }

    // 当数据最大容量的超过四分之三时进行扩大
    if (total_size > 0.8 * max_data_size_) {
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
    bool ret = false;
    int pos = hash(op.val.value, this->curr_size_);
    if (buckets_ptr_[pos] == nullptr) {
        init_buckets(pos);
    }

    ret = buckets_ptr_[pos]->invoke(op);
    if (ret == true) {
        when_resize_hash_table();
    }
    return ret;
}

int 
DSHashSet::resize(bool grow) 
{
    if (curr_size_ <= FSET_BUCKETS_INIT_SIZE) {
        if (curr_size_ == 0) { // 当前大小为空时进行初始化
            buckets_ptr_ = new FSet*[FSET_BUCKETS_INIT_SIZE];
            for (int i = 0; i < FSET_BUCKETS_INIT_SIZE; ++i) {
                buckets_ptr_[i] = nullptr;
            }
            curr_size_ = FSET_BUCKETS_INIT_SIZE;
            max_data_size_ = FSET_BUCKETS_INIT_SIZE * FSETNODE_MAX_SIZE;
            return curr_size_;
        }

        // 当库的大小等于初始化大小时，不再进行缩减
        if (grow == false && curr_size_ == FSET_BUCKETS_INIT_SIZE) {
            return curr_size_;
        }
    }

    for (int i = 0; i < curr_size_; ++i) {
        init_buckets(i);
    }

    if (buckets_ptr_ != nullptr) {
        for (int i = 0; i < pred_size_; ++i) {
            auto tmp_bucket_ptr = pred_buckets_ptr_[i];
            pred_buckets_ptr_[i] = nullptr;
            if (tmp_bucket_ptr != nullptr) {
                delete tmp_bucket_ptr;
            }
        }
        delete[] pred_buckets_ptr_;
        pred_buckets_ptr_ = buckets_ptr_;
        buckets_ptr_ = nullptr;
    }

    if (curr_size_ >= FSET_BUCKETS_INIT_SIZE) { 
        pred_size_ = curr_size_;
        curr_size_ = (grow == true ? curr_size_ * 2 : curr_size_ / 2);
    } else {
        curr_size_ = FSET_BUCKETS_INIT_SIZE;
    }
    FSet **tmp_buckets_ptr_ = new FSet*[curr_size_];
    for (int i = 0; i < curr_size_; ++i) {
        tmp_buckets_ptr_[i] = nullptr;
    }
    os::Atomic<FSet**>::compare_and_swap(&buckets_ptr_, nullptr, tmp_buckets_ptr_);

    // 计算当前哈希表存储数据的上限
    // size 桶的数量， FSETNODE_VALUE_MAX_SIZE：FSETNODE单个数组的大小，这个有四个，
    max_data_size_ = curr_size_ * FSETNODE_MAX_SIZE; 
    return curr_size_;
}

void 
DSHashSet::init_buckets(int pos) 
{
    FSet *bucket_ptr = buckets_ptr_[pos];
    if (bucket_ptr == nullptr) {
        FSet *new_bucket_ptr = new FSet();
        if (pred_buckets_ptr_ != nullptr){
            for (int i = 0; i < pred_size_; ++i) {
                if (pred_buckets_ptr_[i] != nullptr) {
                    pred_buckets_ptr_[i]->freeze();
                    pred_buckets_ptr_[i]->node_.split(new_bucket_ptr->node_, pos, curr_size_);
                }
            }
        }
        os::Atomic<FSet*>::compare_and_swap(&buckets_ptr_[pos], nullptr, new_bucket_ptr);
    }
}


void 
DSHashSet::print(void)
{
    int total = 0;
    fprintf(stdout, "================== Bucket Start =================\n");
    for (unsigned i = 0; i < curr_size_; ++i) {
        if (buckets_ptr_[i] == nullptr) {
            continue;
        } else {
            fprintf(stdout, "curr_bucket[%d]: %d\n", i, buckets_ptr_[i]->node_.size());
            buckets_ptr_[i]->node_.print();
            total += buckets_ptr_[i]->node_.size();
        }
    }
    fprintf(stdout, "\n");
    for (unsigned i = 0; i < pred_size_; ++i) {
        if (pred_buckets_ptr_[i] == nullptr) {
            continue;
        } else {
            fprintf(stdout, "pred_bucket[%d]: %d\n", i, pred_buckets_ptr_[i]->node_.size());
            pred_buckets_ptr_[i]->node_.print();
            total += pred_buckets_ptr_[i]->node_.size();
        }
    }
    fprintf(stdout, "Total size: %d\n", total);
    fprintf(stdout, "================== Bucket End =================\n");
}