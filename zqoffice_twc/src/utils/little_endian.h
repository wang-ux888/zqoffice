// =============================================================================
// src/utils/little_endian.h
//
// zq::office::LittleEndian 类
//   getByte / getShort / getUShort / getInt / getInt64
//
// 用于解析 OLE2 / BIFF / Escher 等小端二进制记录
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {

class LittleEndian {
public:
    /// 读取 1 字节
    static unsigned char getByte(unsigned char* b, int offset);

    /// 读取 2 字节有符号 short（小端）
    static short getShort(unsigned char* b, int offset);

    /// 读取 2 字节无符号 ushort（小端）
    static unsigned short getUShort(unsigned char* b, int offset);

    /// 读取 4 字节有符号 int（小端）
    static int getInt(unsigned char* b, int offset);

    /// 读取 8 字节有符号 int64（小端）
    static std::int64_t getInt64(unsigned char* b, int offset);
};

} // namespace office
} // namespace zq
