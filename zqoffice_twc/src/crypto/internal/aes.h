// =============================================================================
// src/crypto/internal/aes.h / aes.cpp
//
// AES-128/192/256 ECB/CBC 实现（FIPS 197）
//
// Office Agile 加密使用 AES-128/192/256-CBC：
//   - AES-128-CBC：Office 2010+ 默认
//   - AES-256-CBC：高安全级别
//
// 接口风格参考 OpenSSL（AES_set_encrypt_key/AES_set_decrypt_key/AES_cbc_encrypt）。
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>

namespace zq {
namespace office {
namespace crypto {
namespace internal {

/// AES 块大小（字节）
constexpr std::size_t kAesBlockSize = 16;

/// AES 密钥扩展后的轮密钥大小（最大 15 轮 × 16 字节 = 240 字节，AES-256）
constexpr std::size_t kAesMaxRoundKeys = 240;

/// AES 轮密钥（按 32 位字存储，最大 60 个字 = 240 字节）
struct AESKey {
    std::uint32_t rd_key[kAesMaxRoundKeys / 4];  // 60 个 32 位字
    int rounds;  // 轮数（10/12/14）
};

/// 设置加密密钥
/// @param key 密钥
/// @param bits 密钥位数（128/192/256）
/// @param aesKey 输出：扩展后的轮密钥
/// @return 0 成功，-1 失败
int aesSetEncryptKey(const void* key, int bits, AESKey& aesKey);

/// 设置解密密钥
/// @param key 密钥
/// @param bits 密钥位数（128/192/256）
/// @param aesKey 输出：扩展后的轮密钥
/// @return 0 成功，-1 失败
int aesSetDecryptKey(const void* key, int bits, AESKey& aesKey);

/// AES 单块加密（16 字节）
/// @param aesKey 轮密钥（已 set_encrypt_key）
/// @param in 输入（16 字节）
/// @param out 输出（16 字节）
void aesEncryptBlock(const AESKey& aesKey, const std::uint8_t in[16], std::uint8_t out[16]);

/// AES 单块解密（16 字节）
/// @param aesKey 轮密钥（已 set_decrypt_key）
/// @param in 输入（16 字节）
/// @param out 输出（16 字节）
void aesDecryptBlock(const AESKey& aesKey, const std::uint8_t in[16], std::uint8_t out[16]);

/// AES-CBC 加密
/// @param aesKey 轮密钥（已 set_encrypt_key）
/// @param iv 初始向量（16 字节，会被修改）
/// @param in 输入（长度需为 16 的倍数）
/// @param out 输出
/// @param len 数据长度（字节）
void aesCbcEncrypt(const AESKey& aesKey, std::uint8_t iv[16],
                   const void* in, void* out, std::size_t len);

/// AES-CBC 解密
/// @param aesKey 轮密钥（已 set_decrypt_key）
/// @param iv 初始向量（16 字节，会被修改）
/// @param in 输入（长度需为 16 的倍数）
/// @param out 输出
/// @param len 数据长度（字节）
void aesCbcDecrypt(const AESKey& aesKey, std::uint8_t iv[16],
                   const void* in, void* out, std::size_t len);

} // namespace internal
} // namespace crypto
} // namespace office
} // namespace zq
