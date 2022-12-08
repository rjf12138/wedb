#include "FSet.h"

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
                    os::Atomic<ssize_t>::fetch_and_sub(&size_, 1);
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
                os::Atomic<uint32_t>::fetch_and_add(&size_, 1);
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
    if (size() == 0) {
        return 0;
    }
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

/////////////////////////////////////////////////////////////////////////////////////////

FSetArray::FSetArray(uint32_t size)
:freeze_(false),
buckets_ptr_(nullptr)
{
    resize(size);
}

FSetArray::~FSetArray(void)
{
    freeze(); // 释放空间前先冻结数组，等待剩余线程处理完任务在释放空间
    if (buckets_ptr_ != nullptr) {
        delete []buckets_ptr_;
        buckets_ptr_ = nullptr;
    }
}

// 存在竞争，如果在exist是调整了大小，insert插入将崩溃
EApplyErr 
FSetArray::invoke(const FSetOp &op, FSetArray &pred_set_array)
{
    int index = hash(op.val.value, bucket_size());
    if (this->node_elem_size(index) == 0) {
        this->init_buckets(index, pred_set_array);
    }

    if (freeze_ == true) {
        return EApplyErr_Freeze;
    }

    bool ret = false;
    if (op.type == EFSetOp_Insert && buckets_ptr_[index].exist(op.val) == false) {
        ret = buckets_ptr_[index].insert(op.val);
    } else if (op.type == EFSetOp_Remove && buckets_ptr_[index].exist(op.val) == true) {
        ret = buckets_ptr_[index].remove(op.val);
    }

    return ret == false ? EApplyErr_Failed : EApplyErr_Ok;
}

bool 
FSetArray::exist(int index, const FSetOp &op)
{
    return buckets_ptr_[index].exist(op.val);
}

uint32_t 
FSetArray::node_elem_size(int index)
{
    return buckets_ptr_[index].size();
}

bool 
FSetArray::freeze(void)
{
    while (freeze_ == false) {
        if (os::Atomic<bool>::compare_and_swap(&freeze_, false, true) == true) {
            return true;
        }
    }
    return false;
}

bool
FSetArray::resize(uint32_t size)
{
    LOG_GLOBAL_INFO("%x size: %u", this, size);
    if (buckets_ptr_ != nullptr) {
        delete []buckets_ptr_;
    }

    bucket_size_ = (size <= FSET_BUCKETS_INIT_SIZE ? FSET_BUCKETS_INIT_SIZE : size);
    buckets_ptr_ = new FSetNode[bucket_size_];
    freeze_ = false;
    return true;
}

EResizeStatus 
FSetArray::when_resize_hash_table(uint32_t elem_size)
{
    for (int i = 0; i < bucket_size(); ++i) {
        // 当某个桶中元素超过四分之三时进行扩大
        if (buckets_ptr_[i].size() > FSETNODE_MAX_SIZE * 0.7) {
            return EResizeStatus_Grow;
        }
    }

    // 当数据的容量小于桶的数量时进行缩减
    if (elem_size < bucket_size() && elem_size > FSET_BUCKETS_INIT_SIZE) {
        return EResizeStatus_Reduce;
    }

    return EResizeStatus_Remain;
}

void 
FSetArray::init_buckets(int pos, FSetArray &pred_set_array)
{
    for (int i = 0; i < pred_set_array.bucket_size(); ++i) {
        pred_set_array.buckets_ptr_[i].split(this->buckets_ptr_[pos], pos, this->bucket_size());
    }
}

void 
FSetArray::print(void)
{
    for (unsigned i = 0; i < this->bucket_size(); ++i) {
        fprintf(stdout, "curr_bucket[%d]: %d\n", i, buckets_ptr_[i].size());
        buckets_ptr_[i].print();
    }
    fprintf(stdout, "\n");
}