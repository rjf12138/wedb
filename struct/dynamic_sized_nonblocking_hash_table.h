#ifndef _DYNAMIC_SIZED__
#define _DYNAMIC_SIZED__
#include "basic/basic_head.h"

enum FSetOpType {
    FSetOpType_Ins,
    FSetOpType_Rem,
};

struct FSetOp {
    FSetOpType type;    // 操作类型
    int key;            // 关键字
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
    bool has_member(int k) {return data_.find(k) != data_.end();}
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
private:
    std::set<int> data_;
    bool ok;
};

//////////////////////////////////////////////////////////
struct HNode {
    std::vector<FSet> buckets_;
    HNode* pred_ptr;

    HNode(void)
    :pred_ptr(nullptr)
    {}
};

class LockFreeDSHSet {
public:
    LockFreeDSHSet(void) 
    :head_ptr(nullptr) {
        
    }
    ~LockFreeDSHSet(void) {}


private:
    HNode *head_ptr;
};
#endif