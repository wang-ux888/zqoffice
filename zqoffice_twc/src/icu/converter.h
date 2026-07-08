// =============================================================================
// src/icu/converter.h
//
// 编码转换器
//
//   1. 内置 UTF-8 ↔ UTF-16LE/BE ↔ Latin-1 转换
//   2. ICU 可用时支持全部 ICU 编码（GBK/Big5/Shift_JIS/EUC-KR/...）
//
// 转换路径：
//   - 内置：UTF-8 ↔ UTF-16LE ↔ UTF-16BE ↔ Latin-1
//   - ICU：  任意编码 → UTF-16 → 任意编码
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "charset_detector.h"

namespace zq {
namespace office {
namespace icu {

/// 编码转换结果
struct ConvertResult {
    bool success = false;                  ///< 是否成功
    std::vector<unsigned char> data;       ///< 转换后的字节
    std::size_t bytesRead = 0;             ///< 已读取的源字节数
    std::size_t bytesWritten = 0;          ///< 已写入的目标字节数
    std::string errorMessage;              ///< 错误信息

    /// 转换为字符串视图（便于文本处理）
    std::string asString() const {
        return std::string(reinterpret_cast<const char*>(data.data()), data.size());
    }
};

/// 编码转换器
class Converter {
public:
    /// 通用编码转换
    /// @param data       源数据
    /// @param size       源字节数
    /// @param from       源编码
    /// @param to         目标编码
    /// @return 转换结果
    static ConvertResult convert(const unsigned char* data, std::size_t size,
                                  Charset from, Charset to);

    /// 转换为 UTF-8（便捷接口）
    static ConvertResult toUtf8(const unsigned char* data, std::size_t size, Charset from);

    /// 从 UTF-8 转换（便捷接口）
    static ConvertResult fromUtf8(const unsigned char* data, std::size_t size, Charset to);

    /// 转换为 UTF-8 字符串（便捷接口）
    /// @return 成功返回转换后的字符串，失败返回空字符串
    static std::string toUtf8String(const unsigned char* data, std::size_t size, Charset from);

    // -----------------------------------------------------------------------
    // 内置转换函数（不依赖 ICU）
    // -----------------------------------------------------------------------

    /// UTF-16LE → UTF-8
    static ConvertResult utf16leToUtf8(const unsigned char* data, std::size_t size);

    /// UTF-16BE → UTF-8
    static ConvertResult utf16beToUtf8(const unsigned char* data, std::size_t size);

    /// UTF-8 → UTF-16LE
    static ConvertResult utf8ToUtf16le(const unsigned char* data, std::size_t size);

    /// UTF-8 → UTF-16BE
    static ConvertResult utf8ToUtf16be(const unsigned char* data, std::size_t size);

    /// Latin-1 → UTF-8
    static ConvertResult latin1ToUtf8(const unsigned char* data, std::size_t size);

    /// UTF-8 → Latin-1（超出 Latin-1 范围的字符替换为 '?'）
    static ConvertResult utf8ToLatin1(const unsigned char* data, std::size_t size);

    /// ASCII → UTF-8（实际上是直接拷贝，ASCII 是 UTF-8 的子集）
    static ConvertResult asciiToUtf8(const unsigned char* data, std::size_t size);

private:
    /// 编码 UTF-8 码点到字节流
    static void encodeUtf8(std::uint32_t cp, std::vector<unsigned char>& out);

    /// 编码 UTF-16 码点（处理 surrogate pair）
    static void encodeUtf16le(std::uint32_t cp, std::vector<unsigned char>& out);
    static void encodeUtf16be(std::uint32_t cp, std::vector<unsigned char>& out);
};

} // namespace icu
} // namespace office
} // namespace zq
