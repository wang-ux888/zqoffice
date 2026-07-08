// =============================================================================
// src/icu/charset_detector.h
//
// 字符集检测器
//
//   1. BOM 检测（确定性识别）
//   2. 启发式检测（UTF-8 严校验、UTF-16 模式识别）
//   3. ICU 统计检测（当 ICU 可用时）
//
// 检测优先级：BOM > UTF-8 严校验 > UTF-16 模式 > ICU 统计 > 默认 Latin-1
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace zq {
namespace office {
namespace icu {

/// 字符编码类型
enum class Charset {
    Unknown,        ///< 未知
    UTF8,           ///< UTF-8（无 BOM）
    UTF8_BOM,       ///< UTF-8 with BOM
    UTF16LE,        ///< UTF-16 Little Endian（无 BOM）
    UTF16BE,        ///< UTF-16 Big Endian（无 BOM）
    UTF16LE_BOM,    ///< UTF-16LE with BOM
    UTF16BE_BOM,    ///< UTF-16BE with BOM
    UTF32LE,        ///< UTF-32 Little Endian with BOM
    UTF32BE,        ///< UTF-32 Big Endian with BOM
    Latin1,         ///< ISO-8859-1（西欧 Latin-1）
    GBK,            ///< GBK（简体中文）
    GB18030,        ///< GB18030（中文超集）
    Big5,           ///< Big5（繁体中文）
    Shift_JIS,      ///< Shift_JIS（日文）
    EUC_KR,         ///< EUC-KR（韩文）
    ASCII,          ///< 纯 ASCII
};

/// 检测结果
struct DetectionResult {
    Charset charset = Charset::Unknown;  ///< 检测到的编码
    int confidence = 0;                   ///< 置信度（0-100）
    std::string charsetName;              ///< 编码名称（ICU 风格，如 "UTF-8", "GB18030"）
    std::size_t bomSize = 0;              ///< BOM 字节数（0 表示无 BOM）

    /// 是否有效
    bool isValid() const { return charset != Charset::Unknown; }

    /// 是否有 BOM
    bool hasBOM() const { return bomSize > 0; }
};

/// 字符集检测器
class CharsetDetector {
public:
    /// 检测字符编码
    /// @param data  数据指针
    /// @param size  数据字节数
    /// @return 检测结果
    static DetectionResult detect(const unsigned char* data, std::size_t size);

    /// 检测字符编码（重载，接受 const char*）
    static DetectionResult detect(const char* data, std::size_t size);

    /// 获取编码的标准名称字符串
    static const char* charsetName(Charset cs);

    /// 根据 BOM 获取编码
    /// @return 匹配的编码，BOM 长度通过 bomSize 返回
    static Charset detectBOM(const unsigned char* data, std::size_t size, std::size_t& bomSize);

    /// 严校验 UTF-8
    /// @return 全部为有效 UTF-8 字节序列返回 true
    static bool validateUTF8(const unsigned char* data, std::size_t size);

    /// 检测是否为纯 ASCII
    static bool isASCII(const unsigned char* data, std::size_t size);

    /// 检测是否为 UTF-16LE/BE（基于 null 字节模式）
    /// @return Charset::UTF16LE / UTF16BE / Unknown
    static Charset detectUTF16Heuristic(const unsigned char* data, std::size_t size);
};

} // namespace icu
} // namespace office
} // namespace zq
