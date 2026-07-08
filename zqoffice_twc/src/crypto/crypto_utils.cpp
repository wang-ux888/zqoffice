// =============================================================================
// src/crypto/crypto_utils.cpp
//
// CryptoUtils 实现
// =============================================================================
#include "crypto_utils.h"

#include "internal/base64.h"

namespace zq {
namespace office {
namespace crypto {

std::string CryptoUtils::DecodeBase64(const std::string& encoded) {
    return internal::base64Decode(encoded);
}

std::string CryptoUtils::EncodeBase64(const std::string& data) {
    return internal::base64Encode(data);
}

std::string CryptoUtils::Utf8ToUtf16(const std::string& utf8) {
    std::string result;
    result.reserve(utf8.size() * 2);

    auto appendU16 = [&result](std::uint16_t v) {
        result.push_back(static_cast<char>(v & 0xFF));         // low byte
        result.push_back(static_cast<char>((v >> 8) & 0xFF));  // high byte
    };

    auto appendU16Pair = [&result](std::uint16_t hi, std::uint16_t lo) {
        // UTF-16 代理对：U+10000..U+10FFFF
        // 高代理：0xD800 + ((cp - 0x10000) >> 10)
        // 低代理：0xDC00 + ((cp - 0x10000) & 0x3FF)
        result.push_back(static_cast<char>(hi & 0xFF));
        result.push_back(static_cast<char>((hi >> 8) & 0xFF));
        result.push_back(static_cast<char>(lo & 0xFF));
        result.push_back(static_cast<char>((lo >> 8) & 0xFF));
    };

    std::size_t i = 0;
    while (i < utf8.size()) {
        std::uint8_t b0 = static_cast<std::uint8_t>(utf8[i]);
        std::uint32_t cp;

        if (b0 < 0x80) {
            cp = b0;
            ++i;
        } else if ((b0 & 0xE0) == 0xC0) {
            // 2 字节
            if (i + 1 >= utf8.size()) break;
            std::uint8_t b1 = static_cast<std::uint8_t>(utf8[i + 1]);
            cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0) {
            // 3 字节
            if (i + 2 >= utf8.size()) break;
            std::uint8_t b1 = static_cast<std::uint8_t>(utf8[i + 1]);
            std::uint8_t b2 = static_cast<std::uint8_t>(utf8[i + 2]);
            cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0) {
            // 4 字节
            if (i + 3 >= utf8.size()) break;
            std::uint8_t b1 = static_cast<std::uint8_t>(utf8[i + 1]);
            std::uint8_t b2 = static_cast<std::uint8_t>(utf8[i + 2]);
            std::uint8_t b3 = static_cast<std::uint8_t>(utf8[i + 3]);
            cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) |
                 ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            i += 4;
        } else {
            // 非法 UTF-8
            ++i;
            continue;
        }

        if (cp <= 0xFFFF) {
            appendU16(static_cast<std::uint16_t>(cp));
        } else {
            // 代理对
            cp -= 0x10000;
            std::uint16_t hi = static_cast<std::uint16_t>(0xD800 + (cp >> 10));
            std::uint16_t lo = static_cast<std::uint16_t>(0xDC00 + (cp & 0x3FF));
            appendU16Pair(hi, lo);
        }
    }
    return result;
}

std::string CryptoUtils::Utf16ToUtf8(const std::string& utf16le) {
    std::string result;
    result.reserve(utf16le.size() / 2);

    auto appendUtf8 = [&result](std::uint32_t cp) {
        if (cp < 0x80) {
            result.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    };

    std::size_t i = 0;
    while (i + 1 < utf16le.size()) {
        std::uint16_t v = static_cast<std::uint8_t>(utf16le[i]) |
                          (static_cast<std::uint8_t>(utf16le[i + 1]) << 8);
        i += 2;

        if (v >= 0xD800 && v <= 0xDBFF) {
            // 高代理，需要低代理
            if (i + 1 >= utf16le.size()) break;
            std::uint16_t lo = static_cast<std::uint8_t>(utf16le[i]) |
                               (static_cast<std::uint8_t>(utf16le[i + 1]) << 8);
            i += 2;
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                std::uint32_t cp = 0x10000 + ((static_cast<std::uint32_t>(v - 0xD800)) << 10) +
                                   (lo - 0xDC00);
                appendUtf8(cp);
            }
        } else {
            appendUtf8(v);
        }
    }
    return result;
}

} // namespace crypto
} // namespace office
} // namespace zq
