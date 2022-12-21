#include "sstable.h"

SStable::SStable()
:mem_db_(nullptr)
{
}

SStable::~SStable(void)
{
}

int 
SStable::write_to_file(MemoryDB *mem_db)
{

}

int 
SStable::read_from_file(MemoryDB *mem_db)
{

}

int 
SStable::create_data_block(void)
{
    if (mem_db_ == nullptr) {
        return -1;
    }

    basic::ByteBuffer data_block;
    basic::ByteBuffer filter_block;

    // 创建 data_block
    for (auto iter = mem_db_->mem_db_.begin(); iter != mem_db_->mem_db_.end(); ++iter) {
        // 写入key
        basic::ByteBuffer key_buff = iter.key().to_record();
        data_block.write_int32(key_buff.data_size());

        auto beg_iter = key_buff.begin();
        key_buff.get_data(data_block, beg_iter, key_buff.data_size());

        // 写入value
        beg_iter = iter.value().begin();
        iter.value().get_data(data_block, beg_iter, iter.value().data_size());
    }

    
}

int 
SStable::create_filter_block(void)
{

}

int 
SStable::create_meta_index_block(void)
{

}

int 
SStable::create_footer(void)
{

}