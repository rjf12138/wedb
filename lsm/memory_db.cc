#include "memory_db.h"
#include "error_code.h"

#include "basic/logger.h"


KeyValueBlock::KeyValueBlock(void)
:mark_(0)
{
}

KeyValueBlock::KeyValueBlock(const std::string &key, const basic::ByteBuffer& value)
:mark_(0),
key_(key),
value_(value)
{
}

KeyValueBlock::~KeyValueBlock(void)
{
}

bool 
KeyValueBlock::put(const std::string &key, const basic::ByteBuffer& value)
{
    // 检查关键字和值得长度
    if (key.length() > MAX_TWO_BYTE_LENGTH || value.data_size() > MAX_THREE_BYTE_LENGTH) {
        this->set_err_code(ERROR_Code_KeyOrValue_Length_TooLong);
        return false;
    }

    this->reset();

    // 对关键字和值进行赋值
    key_ = key;
    value_ = value;

    this->set_err_code(ERROR_OK);
    return 0;
}

std::string &
KeyValueBlock::key(void)
{
    return key_;
}

basic::ByteBuffer& 
KeyValueBlock::value(void)
{
    return value_;
}

void 
KeyValueBlock::reset(void)
{
    mark_ = 0x00;
    key_.clear();
    value_.clear();
}

basic::ByteBuffer 
KeyValueBlock::to_stream(void)
{
    // 流格式： | mark | 扩展内容长度（可选） | 扩展内容(可选) | key_length | key | value_length  | value | 校验 （默认检查）|
    basic::ByteBuffer buffer;

    // 写入crc校验
    if (this->get_bit(EKeyValueMarkType_Check, 1) != 0) {
        buffer.write_bytes(&crc16_, 2);
    }

    // 将值写入到流中
    buffer += value_;
    uint32_t value_size = value_.data_size();
    if (value_.data_size() <= MAX_ONE_BYTE_LENGTH) {
        this->set_bit(EKeyValueMarkType_ExtendValueLength, EValueLengthExtendType_1bit, 2);
        buffer.write_bytes(&value_size, 1); // 用1个字节表示值长度
    } else if (value_.data_size() <= MAX_TWO_BYTE_LENGTH) {
        this->set_bit(EKeyValueMarkType_ExtendValueLength, EValueLengthExtendType_2bit, 2);
        buffer.write_bytes(&value_size, 2); // 用2个字节表示值长度
    } else {
        this->set_bit(EKeyValueMarkType_ExtendValueLength, EValueLengthExtendType_3bit, 2);
        buffer.write_bytes(&value_size, 3); // 用3个字节表示值长度
    }

    // 将关键字写入到流中
    buffer += key_;
    uint16_t key_size = key_.length();
    if (key_.length() <= MAX_ONE_BYTE_LENGTH) {
        this->set_bit(EKeyValueMarkType_ExtendKeyLength, 0, 1);
        buffer.write_bytes(&key_size, 1);   // 用1个字节表示值长度
    } else {
        this->set_bit(EKeyValueMarkType_ExtendKeyLength, 1, 1);
        buffer.write_bytes(&key_size, 2);   // 用2个字节表示值长度
    }

    // 是否写入扩展内容
    if (this->get_bit(EKeyValueMarkType_Extend, 1) != 0) {
        buffer += extend_data_;

        auto extend_data_size = extend_data_.data_size();
        buffer.write_bytes(&extend_data_size, 1);
    }

    // 写入mark标志字节
    buffer.write_int8(mark_);
}

bool 
KeyValueBlock::from_stream(const basic::ByteBuffer &block)
{
    //TODO
}

void 
KeyValueBlock::set_bit(uint8_t pos, uint8_t value, uint8_t bit_len)
{
    if (bit_len > 8 || bit_len <= 0) {
        return ;
    }

    // 创建一个bit_len长度的掩码 mask，用于获取 value bit_len长度的比特位
    // 然后设到mark_对应的位置中
    uint8_t mask = 0x00;
    for (auto i = 0; i < bit_len; ++i) {
        mask |= (0x01 << i);
    }

    mark_ |= (mask & value) << pos;
}

uint8_t 
KeyValueBlock::get_bit(uint8_t pos, uint8_t bit_len)
{
    if (bit_len > 8 || bit_len <= 0) {
        return ;
    }

    // 创建一个bit_len长度的掩码 mask，用于获取 pos 位置的比特位的值
    uint8_t mask = 0x00;
    for (auto i = 0; i < bit_len; ++i) {
        mask |= (0x01 << i);
    }

    return (mark_ & (mask << pos)) >> pos;
}