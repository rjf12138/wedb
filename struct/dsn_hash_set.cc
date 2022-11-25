#include "dsn_hash_set.h"


DSHashSet::DSHashSet(void)
:elem_size_(0),
curr_buckets_(nullptr),
pred_buckets_(nullptr)
{
    curr_buckets_ = new FSetArray();
    pred_buckets_ = new FSetArray();
}

DSHashSet::~DSHashSet(void) 
{
    if (curr_buckets_ != nullptr) {
        delete curr_buckets_;
        curr_buckets_ = nullptr;
    }

    if (pred_buckets_ != nullptr) {
        delete pred_buckets_;
        pred_buckets_ = nullptr;
    }
}

bool 
DSHashSet::insert(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    op.type = EFSetOp_Insert;

    if (apply(op) == true) {
        os::Atomic<int>::fetch_and_add(&elem_size_, 1);
        return true;
    }
    return false;
}

bool 
DSHashSet::remove(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    op.type = EFSetOp_Remove;

    if (apply(op) == true) {
        os::Atomic<int>::fetch_and_sub(&elem_size_, 1);
        return true;
    }
    return false;
}

bool 
DSHashSet::exist(const int &key) 
{
    FSetOp op;
    op.val.value  = key;
    int pos = hash(key, curr_buckets_->size());
    if (curr_buckets_->set(pos)->exist(op) == true) {
        return true;
    }

    if (pred_buckets_->size() > 0) {
        pos = hash(key, pred_buckets_->size());
        if (pred_buckets_->set(pos)->exist(op) == true) {
            return true;
        }
    }

    return false;
}

uint32_t 
DSHashSet::size(void)
{
    return elem_size_;
}

int 
DSHashSet::when_resize_hash_table(void) 
{
    unsigned total_size = 0;
    for (int i = 0; i < curr_buckets_->size(); ++i) {
        if (pred_buckets_->set(i) == nullptr) {
            return 0;
        }
        total_size += curr_buckets_->node(i)->size();
        // 当某个桶中元素超过四分之三时进行扩大
        if (curr_buckets_->node(i)->size() > FSETNODE_MAX_SIZE * 0.8) {
            return resize(true);
        }
    }

    // 当数据的容量小于桶的数量时进行缩减
    if (elem_size_ < curr_buckets_->size() && elem_size_ > FSET_BUCKETS_INIT_SIZE) {
        return resize(false);
    }

    return 0;
}

bool 
DSHashSet::apply(FSetOp op) 
{
    bool ret = false;
    int pos = hash(op.val.value, curr_buckets_->size());
    if (curr_buckets_->size() == 0) {
        init_buckets(pos);
    }

    while (true) {
        if (curr_buckets_->set(pos) == nullptr) {
            continue;
        }
        ret = curr_buckets_->set(pos)->invoke(op);
        if (ret == EApplyErr_Freeze) {
            continue;
        } else if (ret == EApplyErr_Ok) {
            when_resize_hash_table();
            return true;
        } else {
            return false;
        }
    }
    return false;
}

uint32_t 
DSHashSet::resize(bool grow) 
{
    for (int i = 0; i < curr_buckets_->size(); ++i) {
        init_buckets(i);
    }

    uint32_t new_buckets_size = curr_buckets_->size();
    if (curr_buckets_->size() >= FSET_BUCKETS_INIT_SIZE) { 
        new_buckets_size = (grow == true ? new_buckets_size * 2 : new_buckets_size / 2);
    } else {
        new_buckets_size = FSET_BUCKETS_INIT_SIZE;
    }
    
    if (pred_buckets_->resize(new_buckets_size) == true) {
        auto tmp = pred_buckets_;
        pred_buckets_ = curr_buckets_;
        curr_buckets_ = tmp;
        return new_buckets_size;
    }
    //LOG_GLOBAL_INFO("Create: %d %lx, grow: %s, cas_ret: %d", curr_bucket_size_, pred_buckets_ptr_, grow == true ? "true":"false", ret);

    return 0;
}

void 
DSHashSet::init_buckets(int pos) 
{
    FSet *bucket_ptr = curr_buckets_->set(pos);
    if (bucket_ptr == nullptr) {
        if (pred_buckets_ != nullptr){
            for (int i = 0; i < pred_buckets_->size(); ++i) {
                if (pred_buckets_->set(i) == nullptr) {
                    return;
                }
                pred_buckets_->set(i)->freeze();
                pred_buckets_->node(i)->split(bucket_ptr->node_, pos, curr_buckets_->size());
            }
        }
    }
}


// void 
// DSHashSet::print(void)
// {
//     int total = 0;
//     fprintf(stdout, "================== Bucket Start =================\n");
//     for (unsigned i = 0; i < curr_bucket_size_; ++i) {
//         if (buckets_ptr_[i] == nullptr) {
//             continue;
//         } else {
//             fprintf(stdout, "curr_bucket[%d]: %d\n", i, buckets_ptr_[i]->node_.size());
//             buckets_ptr_[i]->node_.print();
//             total += buckets_ptr_[i]->node_.size();
//         }
//     }
//     fprintf(stdout, "\n");
//     for (unsigned i = 0; i < pred_bucket_size_; ++i) {
//         if (pred_buckets_ptr_[i] == nullptr) {
//             continue;
//         } else {
//             fprintf(stdout, "pred_bucket[%d]: %d\n", i, pred_buckets_ptr_[i]->node_.size());
//             pred_buckets_ptr_[i]->node_.print();
//             total += pred_buckets_ptr_[i]->node_.size();
//         }
//     }
//     fprintf(stdout, "Total size: %d\n", total);
//     fprintf(stdout, "================== Bucket End =================\n");
// }