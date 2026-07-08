// =============================================================================
// src/icu/converter.cpp
// =============================================================================
#include "converter.h"
#include "icu_loader.h"

#include <cstring>

namespace zq {
namespace office {
namespace icu {

// ---------------------------------------------------------------------------
// 公共转换接口
// ---------------------------------------------------------------------------

ConvertResult Converter::convert(const unsigned char* data, std::size_t size,
                                  Charset from, Charset to) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }

    if (from == to) {
        // 相同编码，直接拷贝
        result.data.assign(data, data + size);
        result.bytesRead = size;
        result.bytesWritten = size;
        result.success = true;
        return result;
    }

    // 路径表：直接支持的转换
    if (from == Charset::UTF16LE || from == Charset::UTF16LE_BOM) {
        // 跳过 BOM
        std::size_t offset = (from == Charset::UTF16LE_BOM && size >= 2) ? 2 : 0;
        if (to == Charset::UTF8 || to == Charset::UTF8_BOM) {
            return utf16leToUtf8(data + offset, size - offset);
        }
    }
    if (from == Charset::UTF16BE || from == Charset::UTF16BE_BOM) {
        std::size_t offset = (from == Charset::UTF16BE_BOM && size >= 2) ? 2 : 0;
        if (to == Charset::UTF8 || to == Charset::UTF8_BOM) {
            return utf16beToUtf8(data + offset, size - offset);
        }
    }
    if (from == Charset::UTF8 || from == Charset::UTF8_BOM) {
        std::size_t offset = (from == Charset::UTF8_BOM && size >= 3) ? 3 : 0;
        if (to == Charset::UTF16LE || to == Charset::UTF16LE_BOM) {
            return utf8ToUtf16le(data + offset, size - offset);
        }
        if (to == Charset::UTF16BE || to == Charset::UTF16BE_BOM) {
            return utf8ToUtf16be(data + offset, size - offset);
        }
        if (to == Charset::Latin1) {
            return utf8ToLatin1(data + offset, size - offset);
        }
    }
    if (from == Charset::Latin1) {
        if (to == Charset::UTF8 || to == Charset::UTF8_BOM) {
            return latin1ToUtf8(data, size);
        }
    }
    if (from == Charset::ASCII) {
        if (to == Charset::UTF8 || to == Charset::UTF8_BOM) {
            return asciiToUtf8(data, size);
        }
        if (to == Charset::Latin1) {
            // ASCII 是 Latin-1 的子集
            result.data.assign(data, data + size);
            result.bytesRead = size;
            result.bytesWritten = size;
            result.success = true;
            return result;
        }
    }

    // 尝试 ICU 转换（当内置不支持时）
    IcuLoader& icu = IcuLoader::instance();
    if (icu.isLoaded() && icu.ucnv_open() && icu.ucnv_close()) {
        const char* fromName = CharsetDetector::charsetName(from);
        const char* toName = CharsetDetector::charsetName(to);

        // 通过 UTF-16 中转：from → UTF-16 → to
        int err = 0;

        // Step 1: from → UTF-16
        void* fromConv = icu.ucnv_open()(fromName, &err);
        if (err != 0 || !fromConv) {
            result.errorMessage = "ICU ucnv_open failed for: " + std::string(fromName);
            return result;
        }

        // 预估 UTF-16 缓冲大小（最坏情况每个字节 → 1 个 UTF-16 单元）
        std::vector<char16_t> utf16Buf(size + 1);
        const char* src = reinterpret_cast<const char*>(data);
        const char* srcLimit = src + size;
        int32_t utf16Len = 0;

        // 使用 ucnv_toUChars（简化接口）
        if (icu.ucnv_toUnicode()) {
            char16_t* dest = utf16Buf.data();
            int32_t destCap = static_cast<int32_t>(utf16Buf.size());
            utf16Len = icu.ucnv_toUnicode()(fromConv, dest, destCap,
                                             &src, srcLimit, nullptr, &err);
        }
        icu.ucnv_close()(fromConv);

        if (err != 0 && err != 15 /* U_BUFFER_OVERFLOW_ERROR */ && err != 11 /* U_STRING_NOT_TERMINATED_WARNING */) {
            result.errorMessage = "ICU toUnicode failed";
            return result;
        }

        // Step 2: UTF-16 → to
        void* toConv = icu.ucnv_open()(toName, &err);
        if (err != 0 || !toConv) {
            result.errorMessage = "ICU ucnv_open failed for: " + std::string(toName);
            return result;
        }

        // 预估输出缓冲大小
        std::vector<char> outBuf(utf16Len * 4 + 16);
        const char16_t* u16src = utf16Buf.data();
        const char16_t* u16srcLimit = u16src + utf16Len;
        int32_t outLen = 0;

        if (icu.ucnv_fromUnicode()) {
            char* dest = outBuf.data();
            int32_t destCap = static_cast<int32_t>(outBuf.size());
            outLen = icu.ucnv_fromUnicode()(toConv, dest, destCap,
                                             &u16src, u16srcLimit, nullptr, &err);
        }
        icu.ucnv_close()(toConv);

        if (err != 0 && err != 15 && err != 11) {
            result.errorMessage = "ICU fromUnicode failed";
            return result;
        }

        result.data.assign(reinterpret_cast<unsigned char*>(outBuf.data()),
                           reinterpret_cast<unsigned char*>(outBuf.data()) + outLen);
        result.bytesRead = size;
        result.bytesWritten = static_cast<std::size_t>(outLen);
        result.success = true;
        return result;
    }

    result.errorMessage = "Unsupported conversion path (ICU not loaded)";
    return result;
}

ConvertResult Converter::toUtf8(const unsigned char* data, std::size_t size, Charset from) {
    return convert(data, size, from, Charset::UTF8);
}

ConvertResult Converter::fromUtf8(const unsigned char* data, std::size_t size, Charset to) {
    return convert(data, size, Charset::UTF8, to);
}

std::string Converter::toUtf8String(const unsigned char* data, std::size_t size, Charset from) {
    ConvertResult r = toUtf8(data, size, from);
    if (!r.success) return std::string();
    return r.asString();
}

// ---------------------------------------------------------------------------
// 内置转换实现
// ---------------------------------------------------------------------------

void Converter::encodeUtf8(std::uint32_t cp, std::vector<unsigned char>& out) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<unsigned char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<unsigned char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<unsigned char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<unsigned char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<unsigned char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<unsigned char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<unsigned char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<unsigned char>(0x80 | (cp & 0x3F)));
    }
}

void Converter::encodeUtf16le(std::uint32_t cp, std::vector<unsigned char>& out) {
    if (cp <= 0xFFFF) {
        out.push_back(static_cast<unsigned char>(cp & 0xFF));
        out.push_back(static_cast<unsigned char>((cp >> 8) & 0xFF));
    } else {
        // Surrogate pair
        cp -= 0x10000;
        std::uint16_t hi = 0xD800 + (cp >> 10);
        std::uint16_t lo = 0xDC00 + (cp & 0x3FF);
        out.push_back(static_cast<unsigned char>(hi & 0xFF));
        out.push_back(static_cast<unsigned char>((hi >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(lo & 0xFF));
        out.push_back(static_cast<unsigned char>((lo >> 8) & 0xFF));
    }
}

void Converter::encodeUtf16be(std::uint32_t cp, std::vector<unsigned char>& out) {
    if (cp <= 0xFFFF) {
        out.push_back(static_cast<unsigned char>((cp >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(cp & 0xFF));
    } else {
        cp -= 0x10000;
        std::uint16_t hi = 0xD800 + (cp >> 10);
        std::uint16_t lo = 0xDC00 + (cp & 0x3FF);
        out.push_back(static_cast<unsigned char>((hi >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(hi & 0xFF));
        out.push_back(static_cast<unsigned char>((lo >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(lo & 0xFF));
    }
}

ConvertResult Converter::utf16leToUtf8(const unsigned char* data, std::size_t size) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }
    if (size % 2 != 0) {
        result.errorMessage = "UTF-16 data size must be even";
        return result;
    }

    std::size_t i = 0;
    while (i + 1 < size) {
        std::uint16_t u = data[i] | (data[i + 1] << 8);
        i += 2;
        std::uint32_t cp = u;

        // Surrogate pair
        if (u >= 0xD800 && u <= 0xDBFF && i + 1 < size) {
            std::uint16_t lo = data[i] | (data[i + 1] << 8);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                i += 2;
            }
        } else if (u >= 0xDC00 && u <= 0xDFFF) {
            // 孤立 low surrogate，跳过
            continue;
        }

        encodeUtf8(cp, result.data);
    }

    result.bytesRead = i;
    result.bytesWritten = result.data.size();
    result.success = true;
    return result;
}

ConvertResult Converter::utf16beToUtf8(const unsigned char* data, std::size_t size) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }
    if (size % 2 != 0) {
        result.errorMessage = "UTF-16 data size must be even";
        return result;
    }

    std::size_t i = 0;
    while (i + 1 < size) {
        std::uint16_t u = (data[i] << 8) | data[i + 1];
        i += 2;
        std::uint32_t cp = u;

        if (u >= 0xD800 && u <= 0xDBFF && i + 1 < size) {
            std::uint16_t lo = (data[i] << 8) | data[i + 1];
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                i += 2;
            }
        } else if (u >= 0xDC00 && u <= 0xDFFF) {
            continue;
        }

        encodeUtf8(cp, result.data);
    }

    result.bytesRead = i;
    result.bytesWritten = result.data.size();
    result.success = true;
    return result;
}

ConvertResult Converter::utf8ToUtf16le(const unsigned char* data, std::size_t size) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }

    std::size_t i = 0;
    while (i < size) {
        unsigned char b = data[i];
        std::uint32_t cp = 0;
        std::size_t seqLen = 0;

        if (b <= 0x7F) {
            cp = b;
            seqLen = 1;
        } else if ((b & 0xE0) == 0xC0) {
            if (i + 1 >= size) break;
            cp = (b & 0x1F) << 6 | (data[i + 1] & 0x3F);
            seqLen = 2;
        } else if ((b & 0xF0) == 0xE0) {
            if (i + 2 >= size) break;
            cp = (b & 0x0F) << 12 | (data[i + 1] & 0x3F) << 6 | (data[i + 2] & 0x3F);
            seqLen = 3;
        } else if ((b & 0xF8) == 0xF0) {
            if (i + 3 >= size) break;
            cp = (b & 0x07) << 18 | (data[i + 1] & 0x3F) << 12
               | (data[i + 2] & 0x3F) << 6 | (data[i + 3] & 0x3F);
            seqLen = 4;
        } else {
            // 非法字节，跳过
            i += 1;
            continue;
        }

        encodeUtf16le(cp, result.data);
        i += seqLen;
    }

    result.bytesRead = i;
    result.bytesWritten = result.data.size();
    result.success = true;
    return result;
}

ConvertResult Converter::utf8ToUtf16be(const unsigned char* data, std::size_t size) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }

    std::size_t i = 0;
    while (i < size) {
        unsigned char b = data[i];
        std::uint32_t cp = 0;
        std::size_t seqLen = 0;

        if (b <= 0x7F) {
            cp = b;
            seqLen = 1;
        } else if ((b & 0xE0) == 0xC0) {
            if (i + 1 >= size) break;
            cp = (b & 0x1F) << 6 | (data[i + 1] & 0x3F);
            seqLen = 2;
        } else if ((b & 0xF0) == 0xE0) {
            if (i + 2 >= size) break;
            cp = (b & 0x0F) << 12 | (data[i + 1] & 0x3F) << 6 | (data[i + 2] & 0x3F);
            seqLen = 3;
        } else if ((b & 0xF8) == 0xF0) {
            if (i + 3 >= size) break;
            cp = (b & 0x07) << 18 | (data[i + 1] & 0x3F) << 12
               | (data[i + 2] & 0x3F) << 6 | (data[i + 3] & 0x3F);
            seqLen = 4;
        } else {
            i += 1;
            continue;
        }

        encodeUtf16be(cp, result.data);
        i += seqLen;
    }

    result.bytesRead = i;
    result.bytesWritten = result.data.size();
    result.success = true;
    return result;
}

ConvertResult Converter::latin1ToUtf8(const unsigned char* data, std::size_t size) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }

    for (std::size_t i = 0; i < size; ++i) {
        encodeUtf8(data[i], result.data);
    }

    result.bytesRead = size;
    result.bytesWritten = result.data.size();
    result.success = true;
    return result;
}

ConvertResult Converter::utf8ToLatin1(const unsigned char* data, std::size_t size) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }

    std::size_t i = 0;
    while (i < size) {
        unsigned char b = data[i];
        std::uint32_t cp = 0;
        std::size_t seqLen = 0;

        if (b <= 0x7F) {
            cp = b;
            seqLen = 1;
        } else if ((b & 0xE0) == 0xC0) {
            if (i + 1 >= size) break;
            cp = (b & 0x1F) << 6 | (data[i + 1] & 0x3F);
            seqLen = 2;
        } else if ((b & 0xF0) == 0xE0) {
            if (i + 2 >= size) break;
            cp = (b & 0x0F) << 12 | (data[i + 1] & 0x3F) << 6 | (data[i + 2] & 0x3F);
            seqLen = 3;
        } else if ((b & 0xF8) == 0xF0) {
            if (i + 3 >= size) break;
            cp = (b & 0x07) << 18 | (data[i + 1] & 0x3F) << 12
               | (data[i + 2] & 0x3F) << 6 | (data[i + 3] & 0x3F);
            seqLen = 4;
        } else {
            i += 1;
            continue;
        }

        // Latin-1 范围 0x00-0xFF，超出替换为 '?'
        if (cp <= 0xFF) {
            result.data.push_back(static_cast<unsigned char>(cp));
        } else {
            result.data.push_back('?');
        }
        i += seqLen;
    }

    result.bytesRead = i;
    result.bytesWritten = result.data.size();
    result.success = true;
    return result;
}

ConvertResult Converter::asciiToUtf8(const unsigned char* data, std::size_t size) {
    ConvertResult result;
    if (!data || size == 0) {
        result.success = true;
        return result;
    }
    // ASCII 是 UTF-8 的子集，直接拷贝
    result.data.assign(data, data + size);
    result.bytesRead = size;
    result.bytesWritten = size;
    result.success = true;
    return result;
}

} // namespace icu
} // namespace office
} // namespace zq
