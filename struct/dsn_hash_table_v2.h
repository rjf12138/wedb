#ifndef _DYNAMIC_SIZED_V2__
#define _DYNAMIC_SIZED_V2__

#include "basic/basic_head.h"
#include "system/system.h"

#define FSETNODE_VALUE_MAX_SIZE     64

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
                while (true) {
                    if (os::Atomic<bool>::compare_and_swap(&values[i].valid, true, false)) {
                        break;
                    }
                }
                return true;
            }
        }

        return false;
    }

    bool insert(const FSetValue &val) {
        for (int i = 0; i < FSETNODE_VALUE_MAX_SIZE; ++i) {
            if (values[i].valid == false) {
                while (true) {
                    if (os::Atomic<bool>::compare_and_swap(&values[i].valid, false, true)) {
                        break;
                    }
                }
                values[i].value = val.value;
                return true;
            }
        }

        return false;
    }
};

class FSet {
public:
    FSet(void) 
    :flag_(false) {}

    bool invoke(FSetOp op) {
        
    }

    void do_freeze(void) {
        while (node_.op.type != EFSetOp_WaitFree) {
            FSetOp &tmp = node_.op;
            if (tmp.type == EFSetOp_None) {
                if (os::Atomic<EFSetOp>::compare_and_swap(&tmp.type, EFSetOp_None, EFSetOp_WaitFree)) {
                    break;
                }
            } else {
                help_finish();
            }
        }
    }

    void help_finish(void) {
        FSetOp &op = node_.op;
        if (op.type != EFSetOp_None && op.type != EFSetOp_WaitFree) {
            if (op.type == EFSetOp_Insert) {
                op.resp = node_.exist(op.val);
                node_.insert(op.val);
            } else {
                op.resp = node_.exist(op.val);
                node_.remove(op.val);
            }
            op.prio = -1; // -1表示∞
        }
    }
private:
    FSetNode node_;
    bool flag_;
};

#endif