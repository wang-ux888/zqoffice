// =============================================================================
// src/icu/charset_detector.cpp
// =============================================================================
#include "charset_detector.h"
#include "icu_loader.h"

namespace zq {
namespace office {
namespace icu {

// ---------------------------------------------------------------------------
// 公共接口
// ---------------------------------------------------------------------------

DetectionResult CharsetDetector::detect(const unsigned char* data, std::size_t size) {
    DetectionResult result;
    if (!data || size == 0) {
        result.charset = Charset::Unknown;
        result.confidence = 0;
        return result;
    }

    // 1. BOM 检测（确定性）
    std::size_t bomSize = 0;
    Charset bomCharset = detectBOM(data, size, bomSize);
    if (bomCharset != Charset::Unknown) {
        result.charset = bomCharset;
        result.bomSize = bomSize;
        result.confidence = 100;
        result.charsetName = charsetName(bomCharset);
        return result;
    }

    // 2. 检查是否包含 null 字节（null 字节是 UTF-16/UTF-32 的强信号）
    bool hasNull = false;
    for (std::size_t i = 0; i < size; ++i) {
        if (data[i] == 0x00) { hasNull = true; break; }
    }

    // 3. 纯 ASCII 检测（无 null 字节时才检测）
    if (!hasNull && isASCII(data, size)) {
        result.charset = Charset::ASCII;
        result.confidence = 100;
        result.charsetName = "ASCII";
        return result;
    }

    // 4. UTF-16 启发式（含 null 字节时优先检测）
    if (hasNull) {
        Charset utf16 = detectUTF16Heuristic(data, size);
        if (utf16 != Charset::Unknown) {
            result.charset = utf16;
            result.confidence = 80;
            result.charsetName = charsetName(utf16);
            return result;
        }
    }

    // 5. UTF-8 严校验
    if (validateUTF8(data, size)) {
        result.charset = Charset::UTF8;
        result.confidence = 90;
        result.charsetName = "UTF-8";
        return result;
    }

    // 5. ICU 统计检测（当可用时）
    IcuLoader& icu = IcuLoader::instance();
    if (icu.isLoaded() && icu.ucsdet_open() && icu.ucsdet_detect()) {
        int err = 0;
        void* det = icu.ucsdet_open()(&err);
        if (det && err == 0) {
            icu.ucsdet_setText()(det, reinterpret_cast<const char*>(data),
                                 static_cast<int>(size), &err);
            if (err == 0) {
                void* match = icu.ucsdet_detect()(det, &err);
                if (match && err == 0) {
                    const char* name = icu.ucsdet_getName()(match, &err);
                    int conf = icu.ucsdet_getConfidence()(match, &err);
                    if (name) {
                        result.charsetName = name;
                        result.confidence = conf;
                        // 映射 ICU 名称到 Charset 枚举
                        if (result.charsetName == "UTF-8") {
                            result.charset = Charset::UTF8;
                        } else if (result.charsetName == "GB18030") {
                            result.charset = Charset::GB18030;
                        } else if (result.charsetName == "GBK" || result.charsetName == "GB2312") {
                            result.charset = Charset::GBK;
                        } else if (result.charsetName == "Big5") {
                            result.charset = Charset::Big5;
                        } else if (result.charsetName == "Shift_JIS") {
                            result.charset = Charset::Shift_JIS;
                        } else if (result.charsetName == "EUC-KR") {
                            result.charset = Charset::EUC_KR;
                        } else if (result.charsetName == "ISO-8859-1") {
                            result.charset = Charset::Latin1;
                        } else {
                            result.charset = Charset::Unknown;
                        }
                    }
                }
            }
            icu.ucsdet_close()(det);
        }
        if (result.isValid()) return result;
    }

    // 6. 默认：Latin-1
    result.charset = Charset::Latin1;
    result.confidence = 30;
    result.charsetName = "ISO-8859-1";
    return result;
}

DetectionResult CharsetDetector::detect(const char* data, std::size_t size) {
    return detect(reinterpret_cast<const unsigned char*>(data), size);
}

const char* CharsetDetector::charsetName(Charset cs) {
    switch (cs) {
        case Charset::Unknown:    return "Unknown";
        case Charset::UTF8:       return "UTF-8";
        case Charset::UTF8_BOM:   return "UTF-8-BOM";
        case Charset::UTF16LE:    return "UTF-16LE";
        case Charset::UTF16BE:    return "UTF-16BE";
        case Charset::UTF16LE_BOM:return "UTF-16LE-BOM";
        case Charset::UTF16BE_BOM:return "UTF-16BE-BOM";
        case Charset::UTF32LE:    return "UTF-32LE";
        case Charset::UTF32BE:    return "UTF-32BE";
        case Charset::Latin1:     return "ISO-8859-1";
        case Charset::GBK:        return "GBK";
        case Charset::GB18030:    return "GB18030";
        case Charset::Big5:       return "Big5";
        case Charset::Shift_JIS:  return "Shift_JIS";
        case Charset::EUC_KR:     return "EUC-KR";
        case Charset::ASCII:      return "ASCII";
        default:                  return "Unknown";
    }
}

Charset CharsetDetector::detectBOM(const unsigned char* data, std::size_t size,
                                    std::size_t& bomSize) {
    bomSize = 0;
    if (!data || size < 2) return Charset::Unknown;

    // UTF-32LE BOM: FF FE 00 00
    if (size >= 4 && data[0] == 0xFF && data[1] == 0xFE
        && data[2] == 0x00 && data[3] == 0x00) {
        bomSize = 4;
        return Charset::UTF32LE;
    }
    // UTF-32BE BOM: 00 00 FE FF
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00
        && data[2] == 0xFE && data[3] == 0xFF) {
        bomSize = 4;
        return Charset::UTF32BE;
    }
    // UTF-8 BOM: EF BB BF
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        bomSize = 3;
        return Charset::UTF8_BOM;
    }
    // UTF-16LE BOM: FF FE
    if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        bomSize = 2;
        return Charset::UTF16LE_BOM;
    }
    // UTF-16BE BOM: FE FF
    if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
        bomSize = 2;
        return Charset::UTF16BE_BOM;
    }
    // GB18030 BOM: 84 31 95 33
    if (size >= 4 && data[0] == 0x84 && data[1] == 0x31
        && data[2] == 0x95 && data[3] == 0x33) {
        bomSize = 4;
        return Charset::GB18030;
    }

    return Charset::Unknown;
}

bool CharsetDetector::validateUTF8(const unsigned char* data, std::size_t size) {
    if (!data || size == 0) return false;

    std::size_t i = 0;
    while (i < size) {
        unsigned char b = data[i];
        std::size_t seqLen = 0;
        std::uint32_t minVal = 0;

        if (b <= 0x7F) {
            // 0xxxxxxx (ASCII)
            seqLen = 1;
        } else if ((b & 0xE0) == 0xC0) {
            // 110xxxxx 10xxxxxx
            seqLen = 2;
            minVal = 0x80;
        } else if ((b & 0xF0) == 0xE0) {
            // 1110xxxx 10xxxxxx 10xxxxxx
            seqLen = 3;
            minVal = 0x800;
        } else if ((b & 0xF8) == 0xF0) {
            // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            seqLen = 4;
            minVal = 0x10000;
        } else {
            // 非法首字节
            return false;
        }

        // 检查是否有足够的后续字节
        if (i + seqLen > size) return false;

        // 解码并校验后续字节
        std::uint32_t codePoint = 0;
        if (seqLen == 1) {
            codePoint = b;
        } else {
            // 首字节掩码
            std::uint32_t firstMask = (seqLen == 2) ? 0x1F :
                                       (seqLen == 3) ? 0x0F : 0x07;
            codePoint = b & firstMask;
            for (std::size_t j = 1; j < seqLen; ++j) {
                unsigned char cb = data[i + j];
                if ((cb & 0xC0) != 0x80) return false;  // 不是 10xxxxxx
                codePoint = (codePoint << 6) | (cb & 0x3F);
            }
        }

        // 检查 overlong encoding
        if (codePoint < minVal) return false;

        // 检查 surrogates（U+D800 ~ U+DFFF 不允许在 UTF-8 中）
        if (codePoint >= 0xD800 && codePoint <= 0xDFFF) return false;

        // 检查超出 Unicode 范围
        if (codePoint > 0x10FFFF) return false;

        i += seqLen;
    }
    return true;
}

bool CharsetDetector::isASCII(const unsigned char* data, std::size_t size) {
    if (!data || size == 0) return false;
    for (std::size_t i = 0; i < size; ++i) {
        if (data[i] > 0x7F) return false;
    }
    return true;
}

Charset CharsetDetector::detectUTF16Heuristic(const unsigned char* data, std::size_t size) {
    if (!data || size < 4) return Charset::Unknown;

    // 统计 null 字节分布
    std::size_t evenNulls = 0;  // 偶数位置（0,2,4,...）的 null
    std::size_t oddNulls = 0;   // 奇数位置（1,3,5,...）的 null
    std::size_t pairs = size / 2;

    for (std::size_t i = 0; i + 1 < size; i += 2) {
        if (data[i] == 0x00) ++evenNulls;
        if (data[i + 1] == 0x00) ++oddNulls;
    }

    // UTF-16LE: ASCII 字符的 high byte 为 0（偶数位置非 0，奇数位置为 0）
    // UTF-16BE: ASCII 字符的 low byte 为 0（偶数位置为 0，奇数位置非 0）
    if (pairs > 0) {
        double evenRatio = static_cast<double>(evenNulls) / pairs;
        double oddRatio = static_cast<double>(oddNulls) / pairs;

        // 强信号：超过 30% 的奇数位置为 null → UTF-16LE
        if (oddRatio > 0.30 && evenRatio < 0.05) {
            return Charset::UTF16LE;
        }
        // 超过 30% 的偶数位置为 null → UTF-16BE
        if (evenRatio > 0.30 && oddRatio < 0.05) {
            return Charset::UTF16BE;
        }
    }
    return Charset::Unknown;
}

} // namespace icu
} // namespace office
} // namespace zq
