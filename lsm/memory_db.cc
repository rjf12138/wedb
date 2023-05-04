#include "memory_db.h"


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
    if (key.length() > MAX_TWO_BYTE_LENGTH || value.data_size() > MAX_THREE_BYTE_LENGTH) {
        return false;
    }
    key_ = key;
    value_ = value;

    return 0;
}

std::string &
KeyValueBlock::key(void)
{

}

basic::ByteBuffer& 
KeyValueBlock::value(void)
{

}

void 
KeyValueBlock::set_mark(EKeyValueMarkType type, bool value, EValueLengthExtendType value_len_extend_type)
{

}

void 
KeyValueBlock::reset(void)
{

}

basic::ByteBuffer 
KeyValueBlock::to_block(void)
{

}

bool 
KeyValueBlock::from_block(const basic::ByteBuffer &block)
{

}
    