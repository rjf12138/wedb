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
    if (rhs.data.data_size() > this->data.data_size()) {
        return -1;
    } else if (rhs.data.data_size() < this->data.data_size()) {
        return 1;
    } else {
        auto &rhs_data = rhs.data;
        for (int i = 0; i < this->data.data_size(); ++i) {
            if (rhs_data[i] > this->data[i]) {
                return -1;
            } else if (rhs_data[i] < this->data[i]) {
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
    basic::ByteBuffer buffer(data);
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
    buffer.get_data(this->data, start_iter, buffer.data_size() - 8);

    int64_t tmp_data = 0x00;
    buffer.read_int64(tmp_data);

    type = static_cast<DBOperationType>(tmp_data & 0xff);
    seq = static_cast<uint32_t>(tmp_data >> 8);

    return ;
}