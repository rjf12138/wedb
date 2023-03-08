#ifndef __SSTABLE_H__
#define __SSTABLE_H__

#include "memory_db.h"

// 需要有的功能
// 创建

class SStable {
public:
    SStable();
    ~SStable(void);

    int write_to_file(MemoryDB *mem_db);
    int read_from_file(MemoryDB *mem_db);
    
private:
    // 将内存数据同步到文件中
    int minor_compaction(void);
    // 将多个sstable进行合并
    int major_compaction(void);

private:
    MemoryDB *mem_db_;

    basic::ByteBuffer sstable_;
    basic::ByteBuffer data_block_;   // 数据块
    basic::ByteBuffer filter_block_; // 过滤信息块
    basic::ByteBuffer index_block_;  // 保存一个过滤器中最大key以及它在的位置还有这个block的大小
    basic::ByteBuffer meta_index_block_;
};

#endif