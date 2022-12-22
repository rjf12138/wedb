#include "sstable.h"
#include "bloom_filter.h"

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

    basic::ByteBuffer data_block; // 数据块
    basic::ByteBuffer filter_block; // 过滤信息块
    basic::ByteBuffer index_block; // 保存一个过滤器中最大key以及它在的位置还有这个block的大小

    // 创建 data_block
    int data_size = 0;
    BloomFilter bloom_filter;
    for (auto iter = mem_db_->mem_db_.begin(); iter != mem_db_->mem_db_.end(); ++iter) {
        // 写入key长度
        basic::ByteBuffer key_buff = iter.key().to_record();
        data_block.write_int32(key_buff.data_size());

        // 写入key的内容
        auto beg_iter = key_buff.begin();
        key_buff.get_data(data_block, beg_iter, key_buff.data_size());

        // 写入value长度和内容
        data_block.write_int32(iter.value().data_size());
        beg_iter = iter.value().begin();
        iter.value().get_data(data_block, beg_iter, iter.value().data_size());

        bloom_filter.push(iter.key().key);
        data_size += key_buff.data_size() + iter.value().data_size();
        if (data_size > 2048) { // 超过2kb数据创建一个过滤器
            filter_block.write_int32(bloom_filter.result());
            filter_block.clear();
            data_size = 0;
        }
    }
    // 不管filter_block是否为空最后都将剩余的数据进行写入
    filter_block.write_int32(bloom_filter.result());
    filter_block.clear();

    // 在文档中filter_block每个索引数据都要索引位置
    // 但是索引数据是int32位的所以就固定长度读不需要索引数据了
    // 直接写入Base Lg默认值为11，表示每2KB的数据，创建一个新的过滤器来存放过滤数据
    filter_block.write_int8(11);

    basic::ByteBuffer meta_index_block;
    meta_index_block.write_string("filter.bloom_filter");
    // todo: 写入meta的值

    
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