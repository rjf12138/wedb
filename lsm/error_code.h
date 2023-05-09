#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

// 每种错误码最多100个，例如100, 1表示KeyValueBlock的错误类型，后两位则是这个类的错误类型

#define ERROR_OK                            0           // 无错误返回
// KeyValueBlock 类错误码
#define ERROR_Code_LengthTooLong            100         // 关键字或是值得长度超过最大范围
#define ERROR_Code_BlockShortToNomal        101         // 解析关键字/值块失败,块太小了
#define ERROR_Code_ReadCRCFailed            102         // 读取CRC数据失败
#define ERROR_Code_CheckCRCFailed           103         // 校验CRC数据失败
#define ERROR_Code_ReadExtendDataFailed     104         // 读取扩展内容失败
#define ERROR_Code_ReadKeyFailed            105         // 读取关键字失败
#define ERROR_Code_ReadValueFailed          106         // 读取值失败
#define ERROR_Code_ReadValueLengthFailed    107         // 读取值长度失败

#endif