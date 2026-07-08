// =============================================================================
// src/utils/u8string.h
//
// zq::office::U8String 类
//   - Utf8CharBytes  返回 UTF-8 字符占用字节数
//   - SubCharString  按字符数（非字节数）截取子串
//   - StripEndChar   去除末尾指定字符
// =============================================================================
#pragma once

#include <string>
#include <string_view>
#include <cstddef>

namespace zq {
namespace office {

class U8String {
public:
    /// 返回 UTF-8 字符首字节所对应的字符字节数
    /// @param firstByte  UTF-8 字符首字节
    /// @return 1/2/3/4，非法字节返回 1
    static int Utf8CharBytes(unsigned char firstByte);

    /// 返回 UTF-8 字符首字节所对应的字符字节数（指针版本）
    /// @param p  指向 UTF-8 字符首字节的指针
    /// @return 1/2/3/4，非法首字节返回 1
    static int Utf8CharBytes(const char* p);

    /// 返回 UTF-8 字符串的字节长度中包含的字符数
    static std::size_t CharLength(const std::string& s);

    /// @param s   UTF-8 字符串
    /// @param len 字节长度
    /// @return 字符数
    static int CalcCharCount(const char* s, int len);

    /// 字节偏移 → 字符偏移
    /// @param s        UTF-8 字符串
    /// @param len      字节长度
    /// @param bytePos  字节偏移
    /// @return 字符偏移
    static int ByteToCharPos(const char* s, int len, int bytePos);

    /// 字符偏移 → 字节偏移
    /// @param s        UTF-8 字符串
    /// @param len      字节长度
    /// @param charPos  字符偏移
    /// @return 字节偏移
    static int CharPosToByte(const char* s, int len, int charPos);

    /// 是否换行符
    static bool CheckIsLineBreakChar(const char* s);

    /// UTF-8 合法性校验
    static bool CheckValidUTF8String(const char* s);

    /// 是否 UTF-8 字符起始字节
    static bool IsUtf8CharStart(const char* p);

    /// 是否 UTF-8 续接字节（10xxxxxx）
    static bool IsUtf8Char10x(const char* p);

    /// 取第 idx 个 UTF-8 字符（string_view 版本）
    /// @param s    UTF-8 字符串视图
    /// @param idx  字符索引
    /// @return 字符的 string_view，越界返回空
    static std::string_view GetCharAt(std::string_view s, int idx);

    /// 按字符数（非字节数）截取子串
    /// @param s       源 UTF-8 字符串
    /// @param start   起始字符位置（0-based）
    /// @param count   要截取的字符数，std::string::npos 表示到末尾
    /// @return 截取后的 UTF-8 字符串
    static std::string SubCharString(const std::string& s,
                                     std::size_t start,
                                     std::size_t count = std::string::npos);

    /// @param s      UTF-8 字符串
    /// @param len    字节长度
    /// @param start  起始字符位置
    /// @param count  字符数
    /// @return 字符的 string_view
    static std::string_view SubCharString(const char* s, int len,
                                           int start, int count);

    /// @param s      UTF-8 字符串视图
    /// @param start  起始字符位置
    /// @param count  字符数
    /// @return 字符的 string_view
    static std::string_view SubCharString(std::string_view s,
                                           int start, int count);

    /// 去除字符串末尾的指定字符（按字节比较，不区分字符边界）
    static std::string StripEndChar(const std::string& s, char ch);

    /// 去除字符串末尾的指定字符（按 UTF-8 字符比较）
    static std::string StripEndCharUtf8(const std::string& s,
                                        const std::string& oneChar);

    /// 去除末尾 N 个字符（C 字符串版本）
    static std::string_view StripEndChar(const char* s, int len, int count);

    /// 去除末尾 N 个字符（string_view 版本）
    static std::string_view StripEndChar(std::string_view s, int count);
};

} // namespace office
} // namespace zq
