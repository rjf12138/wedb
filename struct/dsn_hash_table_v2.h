#ifndef _DYNAMIC_SIZED_V2__
#define _DYNAMIC_SIZED_V2__

#include "basic/basic_head.h"
#include "system/system.h"

#define FSETNODE_VALUE_MAX_SIZE     1024

enum EFSetOp {
    EFSetOp_None,
    EFSetOp_WaitFree,
    EFSetOp_Insert,
    EFSetOp_Remove,
};

struct FSetValue {
    int value;      // 值
    bool valid;     // 当前值是否有效

    FSetValue(void)
    :value(0),
    valid(false)
    {}

    FSetValue(int val)
    :value(val),
    valid(true)
    {}
};

struct FSetOp {
    EFSetOp type;
    FSetValue val;
    bool resp;
    int prio;

    FSetOp(void)
    :type(EFSetOp_None),
    resp(false),
    prio(0)
    {}
};

struct FSetNode {
    FSetValue values[FSETNODE_VALUE_MAX_SIZE];
    FSetOp op;

    FSetNode(void)
    :size_(0)
    {}

    bool exist(const FSetValue &val) {
        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values[i].valid == true && val.value == values[i].value) {
                return true;
            }
        }

        return false;
    }

    bool remove(const FSetValue &val) {
        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values[i].valid == true && val.value == values[i].value) {
                if (os::Atomic<bool>::compare_and_swap(&values[i].valid, true, false)) {
                    os::Atomic<int>::fetch_and_sub(&size_, 1);
                    return true;
                }
            }
        }
        return false;
    }

    bool insert(const FSetValue &val) {
        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values[i].valid == false) {
                values[i].valid = true;
                values[i].value = val.value;
                ++size_;
                return true;
                // if (os::Atomic<bool>::compare_and_swap(&values[i].valid, false, true)) {
                //     values[i].value = val.value;
                //     os::Atomic<int>::fetch_and_add(&size_, 1);
                //     return true;
                // }
            }
        }

        return false;
    }

    uint32_t size(void) const {return size_;}
private:
    uint32_t size_;
};

// class FSet {
// public:
//     FSet(void) 
//     :flag_(false) {}

//     bool invoke(FSetOp op) {
//         return false;
//     }

//     void do_freeze(void) {
//         while (node_.op.type != EFSetOp_WaitFree) {
//             FSetOp &tmp = node_.op;
//             if (tmp.type == EFSetOp_None) {
//                 if (os::Atomic<EFSetOp>::compare_and_swap(&tmp.type, EFSetOp_None, EFSetOp_WaitFree)) {
//                     break;
//                 }
//             } else {
//                 help_finish();
//             }
//         }
//     }

//     void help_finish(void) {
//         FSetOp &op = node_.op;
//         if (op.type != EFSetOp_None && op.type != EFSetOp_WaitFree) {
//             if (op.type == EFSetOp_Insert) {
//                 op.resp = node_.exist(op.val);
//                 node_.insert(op.val);
//             } else {
//                 op.resp = node_.exist(op.val);
//                 node_.remove(op.val);
//             }
//             op.prio = -1; // -1表示∞
//         }
//     }
// private:
//     FSetNode node_;
//     bool flag_;
// };

#endif