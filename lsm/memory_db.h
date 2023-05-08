#ifndef __MEMORY_DB_H__
#define __MEMORY_DB_H__

#include "basic/byte_buffer.h"
#include "basic/err_handle.h"
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
//4. value长度扩展: 3和4表示值得长度，01：一个字节表示长度，10：两个字节表示长度，11：3个字节表示长度。
//	（注：虽然值的最大长度能用3个字节表示，不过单个value的最大不能超过16MB。234位会根据内容长度自动设置）
//5. 压缩位：该位设置后会对value进行压缩
//6. 校验位：该位设置后，会对关键字和值进行校验检查内容是否正确（CRC校验）
//7. 操作表示位：表示当前值是增加，删除。 0：增加, 1: 删除。（这两位必须设置）
//8. 保留

enum EKeyValueMarkType {
    EKeyValueMarkType_Extend = 1,
    EKeyValueMarkType_ExtendKeyLength = 2,
    EKeyValueMarkType_ExtendValueLength = 3,
    EKeyValueMarkType_Compress = 5,
    EKeyValueMarkType_Check = 6,
    EKeyValueMarkType_Operation = 7,
    EKeyValueMarkType_Reserved = 8,
};

enum EValueLengthExtendType {
    EValueLengthExtendType_1bit = 1,
    EValueLengthExtendType_2bit = 2,
    EValueLengthExtendType_3bit = 3,
};

#define MAX_ONE_BYTE_LENGTH     0xFF
#define MAX_TWO_BYTE_LENGTH     0xFFFF
#define MAX_THREE_BYTE_LENGTH   0xFFFFFF


class KeyValueBlock : public basic::ErrHandle {
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

    // 设置mark_中某段位置的比特位
    inline void set_bit(uint8_t pos, uint8_t value, uint8_t bit_len);
    // 获取mark_某段位置的比特位
    inline uint8_t get_bit(uint8_t pos, uint8_t bit_len);
    
    // 设置校验
    void check_crc16(void) {}
    // 设置扩展内容
    void set_extend_data(basic::ByteBuffer extends_data) {}
    // 获取扩展内容
    basic::ByteBuffer& get_extend_data(void);

    // 重置键值对
    void reset(void);

    // 生成一个键值数据块
    basic::ByteBuffer to_stream(void);
    // 从键值数据块获取数据
    bool from_stream(const basic::ByteBuffer &block);

private:
    uint8_t mark_;
    uint16_t crc16_;

    std::string key_;
    basic::ByteBuffer value_;

    basic::ByteBuffer extend_data_;
};
#endif