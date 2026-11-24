#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

// 每种错误码按模块分段，方便定位。

#define ERROR_OK                            0           // 无错误返回

// 通用 LSM 错误码
#define ERROR_Code_IO                       200         // 文件/目录/POSIX 操作失败
#define ERROR_Code_Corruption               201         // CRC、长度、格式校验失败
#define ERROR_Code_NotFound                 202         // key 不存在
#define ERROR_Code_InvalidArgument          203         // 空 key、超长 key/value、参数错误
#define ERROR_Code_Locked                   204         // DB 已被其它进程打开
#define ERROR_Code_Unsupported              205         // 未知格式版本/未实现特性
#define ERROR_Code_AlreadyExists            206         // 重复创建/文件编号冲突
#define ERROR_Code_Internal                 207         // 内部状态错误

#endif
