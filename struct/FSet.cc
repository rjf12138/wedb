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
    //LOG_GLOBAL_INFO("Create %d", index_);
}

FSet::~FSet(void) 
{
    //LOG_GLOBAL_INFO("Destroy %d", index_);
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

FSetArray::FSetArray(uint32_t size)
:freeze_(false)
{
    size_ = (size <= FSET_BUCKETS_INIT_SIZE ? FSET_BUCKETS_INIT_SIZE : size);
    buckets_ptr_ = new FSet[size_];
}

FSetArray::FSetArray(FSetArray &farr)
{
    copy(farr);
}

FSetArray::~FSetArray(void)
{
    if (buckets_ptr_ != nullptr) {
        delete []buckets_ptr_;
    }
}

FSetNode *
FSetArray::node(int index)
{
    if (freeze_ == true || index >= size_) {
        return nullptr;
    }
    return &(buckets_ptr_[index].node_);
}

FSet* 
FSetArray::set(int index)
{
    if (freeze_ == true || index >= size_) {
        return nullptr;
    }
    return &(buckets_ptr_[index]);
}

void 
FSetArray::freeze(void)
{
    for (uint32_t i = 0; i < size_; ++i) {
        buckets_ptr_[i].freeze();
    }
}

int FSetArray::copy(FSetArray &farr)
{
    farr.freeze();
}