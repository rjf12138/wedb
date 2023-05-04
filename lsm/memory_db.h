#ifndef __MEMORY_DB_H__
#define __MEMORY_DB_H__

#include "basic/byte_buffer.h"
#include "data_structure/data_struct.h"

enum EDBOperationType {
    DBOperationType_Add,
    DBOperationType_Modify,
    DBOperationType_Remove,
};

//
//1. 扩展位：用于扩展mark字节作用，扩展内容不应超过一个字节所能表达的长度范围，扩展内容设定和解析自定义
//2. key长度扩展: 默认key的长度最大不能超过一个字节所能表示的范围，设置该位后，key的长度范围是两个字节能表示的范围
//3. 
//4. value长度扩展: 3和4表示值得长度，00：一个字节表示长度，01：两个字节表示长度，10：3个字节表示长度。
//	（注：虽然值的最大长度能用3个字节表示，不过单个value的最大不能超过16MB。234位会根据内容长度自动设置）
//5. 压缩位：该位设置后会对value进行压缩
//6. 校验位：该位设置后，会对关键字和值进行校验检查内容是否正确（CRC校验）
//7. 操作表示位：表示当前值是增加，删除。 0：增加, 1: 删除。（这两位必须设置）
//8. 保留

enum EKeyValueMarkType {
    EKeyValueMarkType_Extend = 0,
    EKeyValueMarkType_ExtendKeyLength = 1,
    EKeyValueMarkType_ExtendValueLength = 2,
    EKeyValueMarkType_Compress = 4,
    EKeyValueMarkType_Check = 5,
    EKeyValueMarkType_Operation = 6,
    EKeyValueMarkType_Reserved = 7,
};

enum EValueLengthExtendType {
    EValueLengthExtendType_1bit,
    EValueLengthExtendType_2bit,
    EValueLengthExtendType_3bit,
};

#define MAX_ONE_BYTE_LENGTH     0xFF
#define MAX_TWO_BYTE_LENGTH     0xFFFF
#define MAX_THREE_BYTE_LENGTH   0xFFFFFF


class KeyValueBlock {
public:
    KeyValueBlock(void);
    KeyValueBlock(const std::string &key, const basic::ByteBuffer& value);
    ~KeyValueBlock(void);

    // 放入一个键值对
    bool put(const std::string &key, const basic::ByteBuffer& value);

    // 获取关键字
    std::string &key(void);
    // 获取值
    basic::ByteBuffer& value(void);
    // 设置mark标志位
    void set_mark(EKeyValueMarkType type, bool value, EValueLengthExtendType value_len_extend_type = EValueLengthExtendType_1bit);
    
    // 重置键值对
    void reset(void);

    // 生成一个键值数据块
    basic::ByteBuffer to_block(void);
    // 从键值数据块获取数据
    bool from_block(const basic::ByteBuffer &block);

private:
    uint8_t mark_;

    std::string key_;
    basic::ByteBuffer value_;
};
#endif