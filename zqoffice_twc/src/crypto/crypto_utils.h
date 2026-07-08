// =============================================================================
// src/crypto/crypto_utils.h / crypto_utils.cpp
//
// CryptoUtils 类（全 static，加密相关工具函数）
//
//   - DecodeBase64：Base64 解码
//   - Utf8ToUtf16：UTF-8 → UTF-16LE（Office 加密场景使用 UTF-16LE）
//   - ParseCipherAlgorithm / ParseCipherChaining / ParseHashAlgorithm
//     （在 crypto_enums.h 中已实现为 inline，此处通过 CryptoUtils 包装暴露）
//   - RotateLeft / RotateRight：8-bit 循环移位（用于 XOR Obfuscation）
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "crypto_enums.h"

namespace zq {
namespace office {
namespace crypto {

/// 加密工具类（全 static）
class CryptoUtils {
public:
    /// Base64 解码
    static std::string DecodeBase64(const std::string& encoded);

    /// Base64 编码
    static std::string EncodeBase64(const std::string& data);

    /// UTF-8 → UTF-16LE
    /// @param utf8 UTF-8 字符串
    /// @return UTF-16LE 字节流（每个字符 2 字节，小端）
    static std::string Utf8ToUtf16(const std::string& utf8);

    /// UTF-16LE → UTF-8
    /// @param utf16le UTF-16LE 字节流
    /// @return UTF-8 字符串
    static std::string Utf16ToUtf8(const std::string& utf16le);

    /// 解析加密算法名（包装 crypto_enums.h 中的 parseCipherAlgorithm）
    static CipherAlgorithm ParseCipherAlgorithm(const std::string& s) {
        return parseCipherAlgorithm(s);
    }

    /// 解析链接模式
    static CipherChaining ParseCipherChaining(const std::string& s) {
        return parseCipherChaining(s);
    }

    /// 解析哈希算法名
    static HashAlgorithm ParseHashAlgorithm(const std::string& s) {
        return parseHashAlgorithm(s);
    }

    /// 8-bit 循环左移
    /// @param v 值
    /// @param n 移位数（0-7）
    static std::uint8_t RotateLeft(std::uint8_t v, int n) {
        n &= 7;
        return static_cast<std::uint8_t>((v << n) | (v >> (8 - n)));
    }

    /// 8-bit 循环右移
    static std::uint8_t RotateRight(std::uint8_t v, int n) {
        n &= 7;
        return static_cast<std::uint8_t>((v >> n) | (v << (8 - n)));
    }

    /// 拼接字节为 16 位（小端）
    static std::uint16_t MakeU16LE(std::uint8_t lo, std::uint8_t hi) {
        return static_cast<std::uint16_t>(lo) | (static_cast<std::uint16_t>(hi) << 8);
    }

    /// 拼接字节为 32 位（小端）
    static std::uint32_t MakeU32LE(std::uint8_t b0, std::uint8_t b1,
                                    std::uint8_t b2, std::uint8_t b3) {
        return static_cast<std::uint32_t>(b0) |
               (static_cast<std::uint32_t>(b1) << 8) |
               (static_cast<std::uint32_t>(b2) << 16) |
               (static_cast<std::uint32_t>(b3) << 24);
    }

    /// 取 16 位低字节
    static std::uint8_t LoByte(std::uint16_t v) {
        return static_cast<std::uint8_t>(v & 0xFF);
    }

    /// 取 16 位高字节
    static std::uint8_t HiByte(std::uint16_t v) {
        return static_cast<std::uint8_t>((v >> 8) & 0xFF);
    }
};

} // namespace crypto
} // namespace office
} // namespace zq
