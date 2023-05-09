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
        this->set_err_code(ERROR_Code_LengthTooLong);
        return false;
    }

    this->reset();

    // 对关键字和值进行赋值
    key_ = key;
    value_ = value;

    this->set_err_code(ERROR_OK);
    return 0;
}

basic::ByteBuffer&
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
KeyValueBlock::to_block(void)
{
    // 流格式： | mark | 校验 （默认检查）| 扩展内容长度（可选） | 扩展内容(可选) | key_length | key | value_length  | value |
    basic::ByteBuffer buffer;

    // 写入mark标志字节
    buffer.write_int8(mark_);

    // 写入crc校验
    if (this->get_bit(mark_, EKeyValueMarkType_Check, 1) != 0) {
        buffer.write_bytes(&crc16_, 2);
    }

    // 是否写入扩展内容
    if (this->get_bit(mark_, EKeyValueMarkType_Extend, 1) != 0) {
        auto extend_data_size = extend_data_.data_size();
        buffer.write_bytes(&extend_data_size, 1);

        buffer += extend_data_;
    }

    // 将关键字写入到流中
    uint16_t key_size = key_.data_size();
    if (key_size <= MAX_ONE_BYTE_LENGTH) {
        this->set_bit(mark_, EKeyValueMarkType_ExtendKeyLength, 0, 1);
        buffer.write_bytes(&key_size, 1);   // 用1个字节表示值长度
    } else {
        this->set_bit(mark_, EKeyValueMarkType_ExtendKeyLength, 1, 1);
        buffer.write_bytes(&key_size, 2);   // 用2个字节表示值长度
    }
    buffer += key_;

    // 将值写入到流中
    uint32_t value_size = value_.data_size();
    if (value_.data_size() <= MAX_ONE_BYTE_LENGTH) {
        this->set_bit(mark_, EKeyValueMarkType_ExtendValueLength, EValueLengthExtendType_1bit, 2);
        buffer.write_bytes(&value_size, 1); // 用1个字节表示值长度
    } else if (value_.data_size() <= MAX_TWO_BYTE_LENGTH) {
        this->set_bit(mark_, EKeyValueMarkType_ExtendValueLength, EValueLengthExtendType_2bit, 2);
        buffer.write_bytes(&value_size, 2); // 用2个字节表示值长度
    } else {
        this->set_bit(mark_, EKeyValueMarkType_ExtendValueLength, EValueLengthExtendType_3bit, 2);
        buffer.write_bytes(&value_size, 3); // 用3个字节表示值长度
    }
    buffer += value_;
}

bool 
KeyValueBlock::from_block(basic::ByteBuffer &block)
{
    if (block.data_size() < 3) {
        this->set_err_code(ERROR_Code_BlockShortToNomal);
        return false;
    }

    int read_pos = 0;

    uint8_t mark;
    block.read_only(read_pos++, &mark, sizeof(mark));

    // 读取 crc校验
    uint16_t crc16;
    if (this->get_bit(mark, EKeyValueMarkType_Check, 1) != 0) {
        if (block.data_size() < read_pos + 2) { // 数据不够
            this->set_err_code(ERROR_Code_ReadCRCFailed);
            return false;
        }

        block.read_only(read_pos, &crc16, sizeof(crc16));
        read_pos += 2;

        // 通过crc对剩余数据块进行校验

    }

    // 读取扩展内容
    basic::ByteBuffer extend_data;
    if (this->get_bit(mark, EKeyValueMarkType_Extend, 1) != 0) {
        if (block.data_size() < read_pos + 1) { // 数据不够
            this->set_err_code(ERROR_Code_ReadExtendDataFailed);
            return false;
        }

        // 获取扩展内容长度
        uint8_t extend_data_length = 0;
        block.read_only(read_pos++, &extend_data_length, 1);

        // 检查扩展内容是否有足够的数据
        if (block.data_size() < read_pos + extend_data_length) { // 数据不够
            this->set_err_code(ERROR_Code_ReadExtendDataFailed);
            return false;
        }

        // 读取扩展内容
        auto read_iter = block.begin() + read_pos;
        block.get_data(extend_data, read_iter, extend_data_length);
        read_pos += extend_data_length;
    }

    // 读取关键字
    basic::ByteBuffer key;
    uint16_t key_length = 0;
    int key_length_occupy_bytes = EValueLengthExtendType_1bit;
    if (this->get_bit(mark, EKeyValueMarkType_ExtendKeyLength, 1) != 0) {
        key_length_occupy_bytes = EValueLengthExtendType_2bit;
    }

    if (block.data_size() < read_pos + key_length_occupy_bytes) { // 数据不够
        this->set_err_code(ERROR_Code_ReadKeyFailed);
        return false;
    }

    block.read_only(read_pos, &key_length, key_length_occupy_bytes);
    read_pos += sizeof(key_length_occupy_bytes);

    if (block.data_size() < read_pos + key_length) { // 数据不够
        this->set_err_code(ERROR_Code_ReadKeyFailed);
        return false;
    }

    // 读取关键字内容
    auto read_iter = block.begin() + read_pos;
    block.get_data(key, read_iter, key_length);
    read_pos += key_length;

    // 读取值
    basic::ByteBuffer value;
    uint32_t value_length = 0;
    int value_length_occupy_bytes = this->get_bit(mark, EKeyValueMarkType_ExtendValueLength, 2);
    if (value_length_occupy_bytes == EValueLengthExtendType_0bit) {
        this->set_err_code(ERROR_Code_ReadValueLengthFailed);
        return false;
    }

    if (block.data_size() < read_pos + value_length_occupy_bytes) { // 数据不够
        this->set_err_code(ERROR_Code_ReadValueFailed);
        return false;
    }
    block.read_only(read_pos, &value_length, value_length_occupy_bytes);
    read_pos += sizeof(value_length_occupy_bytes);

    // 读取值内容
    if (block.data_size() < read_pos + value_length) { // 数据不够
        this->set_err_code(ERROR_Code_ReadValueFailed);
        return false;
    }
    read_iter = block.begin() + read_pos;
    block.get_data(key, read_iter, value_length);
    read_pos += value_length;

    // 读取成功覆盖当前类的内容
    mark_ = mark;
    key_ = key;
    value_ = value;
    extend_data_ = extend_data;

    this->set_err_code(ERROR_OK);
    return true;
}
// 打印块内容和解决CRC
void 
KeyValueBlock::print_block(basic::ByteBuffer &block)
{

}

void 
KeyValueBlock::set_bit(uint8_t mark, uint8_t pos, uint8_t value, uint8_t bit_len)
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
KeyValueBlock::get_bit(uint8_t mark, uint8_t pos, uint8_t bit_len)
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