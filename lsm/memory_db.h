#ifndef __MEMORY_DB_H__
#define __MEMORY_DB_H__

#include "basic/byte_buffer.h"
#include "data_structure/data_struct.h"

enum DBOperationType {
    DBOperationType_Add,
    DBOperationType_Modify,
    DBOperationType_Remove,
};

/*
    用户定义的key：这个key值也就是原生的key值；
    序列号：leveldb中，每一次写操作都有一个sequence number，标志着写入操作的先后顺序。
           由于在leveldb中，可能会有多条相同key的数据项同时存储在数据库中，因此需要有一个序列号来标识这些数据项的新旧情况。
           序列号最大的数据项为最新值；
    类型：标志本条数据项的类型，为更新还是删除；
*/

class RecordBlock {
public:
    RecordBlock(void);
    ~RecordBlock(void);

    // 当前类大于传入参数返回：1, 相等返回：0， 小于返回：-1
    int compare(const RecordBlock &lrv, bool is_only_key = false) const;
    // 将所有属性转成一条写入记录
    basic::ByteBuffer to_record(void);
    // 将一条记录转成RecordBlock
    void from_record(basic::ByteBuffer buffer);

public:
    bool operator<(const RecordBlock &rblk) const;
    bool operator>(const RecordBlock &rblk) const;
    bool operator==(const RecordBlock &rblk) const;
    bool operator!=(const RecordBlock &rblk) const;

public:
    basic::ByteBuffer key;
    uint32_t seq;
    DBOperationType type;
};

class MemoryDB {
public:
    MemoryDB(void);
    ~MemoryDB(void);

    // 放入一条记录
    bool put(const basic::ByteBuffer &key, const basic::ByteBuffer &value);
    // 移除一条记录
    bool remove(const basic::ByteBuffer &key);

    // // 检查记录是否存在
    // bool find(const basic::ByteBuffer &key);
    // // 获取记录的值
    // basic::ByteBuffer* get(const basic::ByteBuffer &key);

    // 设置一个之前的的seqence
    void set_last_seq(uint32_t seq) {seq_ = seq;}
    
private:
    basic::ByteBuffer to_stream(void) {}
    void from_stream(void) {}

public:
    ds::SkipList<RecordBlock, basic::ByteBuffer> mem_db_;
    uint32_t seq_;
};

#endif