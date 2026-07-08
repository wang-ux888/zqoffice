// =============================================================================
// src/crypto/internal/sha.h / sha.cpp
//
// SHA-1 / SHA-224 / SHA-256 / SHA-384 / SHA-512 实现
//
// 基于 FIPS 180-4 规范的纯 C++ 实现，无外部依赖。
// 接口风格参考 OpenSSL（SHA1/SHA256/SHA512 函数）。
//
// 用途：
//   - Office Agile 加密（SHA-512 派生密钥 + HMAC）
//   - Office RC4 加密（SHA-1 派生密钥）
//   - Office XOR 加密（不需要 SHA）
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace zq {
namespace office {
namespace crypto {
namespace internal {

// ---------------------------------------------------------------------------
// SHA-1（20 字节摘要）
// ---------------------------------------------------------------------------
/// SHA-1 一次性计算
/// @param data 数据
/// @param len 数据长度
/// @param out 输出缓冲区（至少 20 字节）
void sha1(const void* data, std::size_t len, std::uint8_t out[20]);

/// SHA-1 一次性计算（std::string 版本）
/// @param data 数据
/// @return 20 字节摘要
std::string sha1(const std::string& data);

// ---------------------------------------------------------------------------
// SHA-256（32 字节摘要）
// ---------------------------------------------------------------------------
void sha256(const void* data, std::size_t len, std::uint8_t out[32]);
std::string sha256(const std::string& data);

// ---------------------------------------------------------------------------
// SHA-384（48 字节摘要）
// ---------------------------------------------------------------------------
void sha384(const void* data, std::size_t len, std::uint8_t out[48]);
std::string sha384(const std::string& data);

// ---------------------------------------------------------------------------
// SHA-512（64 字节摘要）
// ---------------------------------------------------------------------------
void sha512(const void* data, std::size_t len, std::uint8_t out[64]);
std::string sha512(const std::string& data);

// ---------------------------------------------------------------------------
// HMAC-SHA1（20 字节 MAC）
// ---------------------------------------------------------------------------
/// HMAC-SHA1
/// @param key 密钥
/// @param keyLen 密钥长度
/// @param data 数据
/// @param dataLen 数据长度
/// @param out 输出（20 字节）
void hmacSha1(const void* key, std::size_t keyLen,
              const void* data, std::size_t dataLen,
              std::uint8_t out[20]);

std::string hmacSha1(const std::string& key, const std::string& data);

// ---------------------------------------------------------------------------
// HMAC-SHA256（32 字节 MAC）
// ---------------------------------------------------------------------------
void hmacSha256(const void* key, std::size_t keyLen,
                const void* data, std::size_t dataLen,
                std::uint8_t out[32]);

std::string hmacSha256(const std::string& key, const std::string& data);

// ---------------------------------------------------------------------------
// HMAC-SHA384（48 字节 MAC）
// ---------------------------------------------------------------------------
void hmacSha384(const void* key, std::size_t keyLen,
                const void* data, std::size_t dataLen,
                std::uint8_t out[48]);

std::string hmacSha384(const std::string& key, const std::string& data);

// ---------------------------------------------------------------------------
// HMAC-SHA512（64 字节 MAC）
// ---------------------------------------------------------------------------
void hmacSha512(const void* key, std::size_t keyLen,
                const void* data, std::size_t dataLen,
                std::uint8_t out[64]);

std::string hmacSha512(const std::string& key, const std::string& data);

} // namespace internal
} // namespace crypto
} // namespace office
} // namespace zq
