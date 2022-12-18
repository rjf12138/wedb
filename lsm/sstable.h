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
};

#endif