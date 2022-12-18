#include "memory_db.h"

RecordBlock::RecordBlock(void)
:seq(0),
type(DBOperationType_Add)
{
}

RecordBlock::~RecordBlock(void)
{}

/*
比较规则：
    首先按照字典序比较用户定义的key（RecordBlock::data），若用户定义key值大，整个internalKey就大；
    若用户定义的key相同，则序列号大的internalKey（RecordBlock::seq）值就小；
*/
int 
RecordBlock::compare(const RecordBlock &rhs)
{
    if (rhs.key.data_size() > this->key.data_size()) {
        return -1;
    } else if (rhs.key.data_size() < this->key.data_size()) {
        return 1;
    } else {
        auto &rhs_data = rhs.key;
        for (int i = 0; i < this->key.data_size(); ++i) {
            if (rhs_data[i] > this->key[i]) {
                return -1;
            } else if (rhs_data[i] < this->key[i]) {
                return 1;
            }
        }

        if (rhs.seq > this->seq) {
            return -1;
        } else if (rhs.seq < this->seq) {
            return 1;
        }
    }

    return 0; // 相等
}

basic::ByteBuffer 
RecordBlock::to_record(void)
{
    // |  key   | seq (7bytes) | type(1byte) |
    basic::ByteBuffer buffer(key);
    int64_t tmp_data = 0x00;
    tmp_data = seq << 8 | type;
    buffer.write_int64(tmp_data);

    return buffer;
}

void 
RecordBlock::from_record(basic::ByteBuffer buffer)
{
    if (buffer.data_size() <= 8) {
        return ;// 小于seq和type的大小说明读写错了
    }

    auto start_iter = buffer.begin();
    buffer.get_data(this->key, start_iter, buffer.data_size() - 8);

    int64_t tmp_data = 0x00;
    buffer.read_int64(tmp_data);

    type = static_cast<DBOperationType>(tmp_data & 0xff);
    seq = static_cast<uint32_t>(tmp_data >> 8);

    return ;
}

///////////////////////// Memory database /////////////////////////////////
MemoryDB::MemoryDB(void)
:seq_(0)
{}

MemoryDB::~MemoryDB(void)
{}

bool 
MemoryDB::put(const basic::ByteBuffer &key, const basic::ByteBuffer &value)
{
    RecordBlock block;
    block.key = key;
    block.seq = ++seq_;
    block.type = DBOperationType_Add;

    return mem_db_.put(block, value);
}


bool 
MemoryDB::remove(const basic::ByteBuffer &key)
{
    RecordBlock block;
    block.key = key;
    block.seq = ++seq_;
    block.type = DBOperationType_Remove;

    return mem_db_.put(block, basic::ByteBuffer());
}

// // 检查记录是否存在
// bool find(const basic::ByteBuffer &key);
// // 获取记录的值
// basic::ByteBuffer* get(const basic::ByteBuffer &key);