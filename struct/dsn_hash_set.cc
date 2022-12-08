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
        // 先将curr_buckets_置为nullptr，防止其他线程再次进入修改curr_buckets_
        // 然后再释放curr_buckets_的内存（现在tmp中），tmp将等待剩余线程处理完事务后退出程序
        auto tmp = curr_buckets_;
        curr_buckets_ = nullptr;
        delete tmp;
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
    int pos = hash(key, curr_buckets_->bucket_size());
    if (curr_buckets_->exist(pos, op) == true) {
        return true;
    }

    if (pred_buckets_->bucket_size() > 0) {
        pos = hash(key, pred_buckets_->bucket_size());
        if (pred_buckets_->exist(pos, op) == true) {
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

bool 
DSHashSet::apply(FSetOp op) 
{
    while (true) {
        EApplyErr ret = curr_buckets_->invoke(op, *pred_buckets_);
        if (ret == EApplyErr_Freeze) {
            continue;
        } else if (ret == EApplyErr_Ok) {
            EResizeStatus is_resize = curr_buckets_->when_resize_hash_table(size());
            if (is_resize != EResizeStatus_Remain) {
                resize(is_resize == EResizeStatus_Grow ? true : false);
            } 
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
    for (int i = 0; i < curr_buckets_->bucket_size(); ++i) {
        curr_buckets_->init_buckets(i, *pred_buckets_);
    }
    
    if (curr_buckets_->freeze() == false) {
        return 0;
    }

    uint32_t new_buckets_size = curr_buckets_->bucket_size();
    if (curr_buckets_->bucket_size() >= FSET_BUCKETS_INIT_SIZE) { 
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
DSHashSet::print(void)
{
    int total = 0;
    fprintf(stdout, "================== Bucket Start =================\n");
    curr_buckets_->print();
    pred_buckets_->print();
    fprintf(stdout, "Total size: %d\n", size());
    fprintf(stdout, "================== Bucket End =================\n");
}