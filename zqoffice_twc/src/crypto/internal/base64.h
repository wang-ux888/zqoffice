// =============================================================================
// src/crypto/internal/base64.h / base64.cpp
//
// Base64 编码/解码（RFC 4648）
//
// Office Agile 加密的 EncryptionInfo XML 中 saltValue/encryptedKeyValue 等字段
// 使用 Base64 编码存储二进制数据。
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace zq {
namespace office {
namespace crypto {
namespace internal {

/// Base64 编码
/// @param data 数据
/// @param len 长度
/// @return Base64 字符串
std::string base64Encode(const void* data, std::size_t len);

/// Base64 编码（std::string 版本）
std::string base64Encode(const std::string& data);

/// Base64 解码
/// @param encoded Base64 字符串
/// @return 解码后的二进制数据
std::string base64Decode(const std::string& encoded);

} // namespace internal
} // namespace crypto
} // namespace office
} // namespace zq
