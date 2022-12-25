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
    int create_data_block(void);
    int create_filter_block(void);
    int create_meta_index_block(void);
    int create_footer(void);

private:
    MemoryDB *mem_db_;

    basic::ByteBuffer sstable_;
    basic::ByteBuffer data_block_;   // 数据块
    basic::ByteBuffer filter_block_; // 过滤信息块
    basic::ByteBuffer index_block_;  // 保存一个过滤器中最大key以及它在的位置还有这个block的大小
    basic::ByteBuffer meta_index_block_;
};

#endif