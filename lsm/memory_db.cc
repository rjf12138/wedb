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
        err_handle_.set_err_code(ERROR_Code_KeyOrValue_Length_TooLong);
        return false;
    }

    // 对关键字和值进行赋值
    key_ = key;
    value_ = value;

    // 设置长度相关的标志位
    if (key_.length() <= MAX_ONE_BYTE_LENGTH) {
        this->set_mark(EKeyValueMarkType_ExtendKeyLength, false);
    } else {
        this->set_mark(EKeyValueMarkType_ExtendKeyLength, true);
    }

    if (value_.data_size() <= MAX_ONE_BYTE_LENGTH) {
        this->set_mark(EKeyValueMarkType_ExtendValueLength, false, EValueLengthExtendType_1bit);
    } else if (value_.data_size() <= MAX_TWO_BYTE_LENGTH) {
        this->set_mark(EKeyValueMarkType_ExtendValueLength, false, EValueLengthExtendType_2bit);
    } else {
        this->set_mark(EKeyValueMarkType_ExtendValueLength, false, EValueLengthExtendType_3bit);
    }

    err_handle_.set_err_code(ERROR_OK);
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
KeyValueBlock::set_mark(EKeyValueMarkType type, bool value, EValueLengthExtendType value_len_extend_type)
{
    switch (type)
    {
    case EKeyValueMarkType_Extend:
    case EKeyValueMarkType_ExtendKeyLength:
    case EKeyValueMarkType_ExtendValueLength:
    case EKeyValueMarkType_Compress:
    case EKeyValueMarkType_Check:
    case EKeyValueMarkType_Operation:
    case EKeyValueMarkType_Reserved:
    default:
        break;
    }
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

}

bool 
KeyValueBlock::from_block(const basic::ByteBuffer &block)
{

}
    