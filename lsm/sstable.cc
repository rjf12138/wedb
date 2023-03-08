#include "sstable.h"
#include "bloom_filter.h"
#include "algorithm/algorithm.h"

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
SStable::minor_compaction(void)
{
    if (mem_db_ == nullptr) {
        return -1;
    }

    // 在文档中filter_block每个索引数据都要索引位置
    // 但是索引数据（布隆过滤器的值）是int32位的所以就固定长度读不需要索引数据了
    // 直接写入Base Lg默认值为11，表示每2KB的数据，创建一个新的过滤器来存放过滤数据
    filter_block_.write_int8(11);

    auto max_key_iter = mem_db_->mem_db_.begin();
    uint32_t max_key_pos = 0;
    uint32_t max_key_length = 0;

    // 创建 data_block
    int data_size = 0;
    BloomFilter bloom_filter;
    for (auto iter = mem_db_->mem_db_.begin(); iter != mem_db_->mem_db_.end(); ++iter) {
        // 获取key值
        basic::ByteBuffer key_buff = iter.key().to_record();

        // 获取布隆过滤器指定范围内最大key
        if (iter.key().compare(max_key_iter.key(), false) == 1) {
            max_key_iter = iter;
            max_key_pos = data_block_.data_size();
            max_key_length = key_buff.data_size();
        }
        // 写入key长度
        data_block_.write_int32(key_buff.data_size());

        // 写入key的内容
        auto beg_iter = key_buff.begin();
        key_buff.get_data(data_block_, beg_iter, key_buff.data_size());

        // 写入value长度和内容
        data_block_.write_int32(iter.value().data_size());
        beg_iter = iter.value().begin();
        iter.value().get_data(data_block_, beg_iter, iter.value().data_size());

        bloom_filter.push(iter.key().key);
        data_size += key_buff.data_size() + iter.value().data_size();
        if (data_size > 2048) { // 超过2kb数据创建一个新的过滤器
            filter_block_.write_int32(bloom_filter.result());
            filter_block_.clear();
            data_size = 0;

            // 一个布隆过滤器内将最大的key值写到index中，格式：| key 的长度（4个字节）| key的具体大小 | key再block中的位置 (4个字节)|
            index_block_.write_int32(max_key_length);
            index_block_ += iter.key().to_record();
            index_block_.write_int32(max_key_pos);
        }
    }
    
    // 不管filter_block是否为空最后都将剩余的数据进行写入
    filter_block_.write_int32(bloom_filter.result());
    bloom_filter.clear();

    meta_index_block_.write_string("filter.bloom_filter");
    int filter_block_pos = 48 + meta_index_block_.data_size() + index_block_.data_size();
    meta_index_block_.write_int32(filter_block_pos); // filter_block 在 sstable 的位置
    meta_index_block_.write_int32(filter_block_.data_size()); // filter_block 的大小

    // 将所有分块合并到一个块中, footer_block
    sstable_.write_int32(49); // meta_index_block 起始位置
    sstable_.write_int32(48 + meta_index_block_.data_size() + 1); // index_block 起始位置
    for (int i = 0; i < 32; ++i) { // 32 个填充字节
        sstable_.write_int8(0x00);
    }
    basic::ByteBuffer inbuff(std::string("rjf12138")), outbuffer;
    algorithm::sha1(inbuff, outbuffer);
    sstable_.write_bytes(outbuffer.str().c_str(), 8); // 写入 8 个字节的 magic num，取 rjf12138 sha1 后的前8个字节

    sstable_ += meta_index_block_;
    sstable_ += index_block_;
    sstable_ += filter_block_;
    sstable_ += data_block_;
}

int 
SStable::major_compaction(void)
{
    
}