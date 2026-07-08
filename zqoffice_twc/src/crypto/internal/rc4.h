// =============================================================================
// src/crypto/internal/rc4.h / rc4.cpp
//
// RC4 流密码实现（RFC 6229）
//
// Office 97-2003 通用 RC4 加密使用此实现。
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>

namespace zq {
namespace office {
namespace crypto {
namespace internal {

/// RC4 状态
struct RC4State {
    std::uint8_t s[256];  // 状态盒
    std::uint8_t i = 0;   // 索引 i
    std::uint8_t j = 0;   // 索引 j
};

/// 初始化 RC4 状态（KSA，Key-Scheduling Algorithm）
/// @param state 状态
/// @param key 密钥
/// @param keyLen 密钥长度（1-256 字节）
void rc4Init(RC4State& state, const void* key, std::size_t keyLen);

/// RC4 加密/解密（PRGA，Pseudo-Random Generation Algorithm）
/// @param state 状态（已 Init）
/// @param data 数据（in-place 加密/解密）
/// @param len 数据长度
void rc4Process(RC4State& state, void* data, std::size_t len);

} // namespace internal
} // namespace crypto
} // namespace office
} // namespace zq
