// =============================================================================
// src/crypto/password_derivation.h / password_derivation.cpp
//
// PasswordDerivation 类（密码派生工具，全 static）
//
// 基于 [MS-OFFCRYPTO] 规范，提供密码派生工具函数：
//   - DeriveXorVerifier：XOR 混淆密码验证子（2.3.7.1）
//   - DeriveRC4Key：RC4 加密密码密钥（2.3.6.1）
//   - DeriveAgileHash：Agile 加密 HMAC 迭代派生（2.3.4.9）
//   - DeriveBlockKey：Agile 加密 blockKey 派生（2.3.4.10）
//   - DeriveSegmentKey：Agile 加密分段密钥（2.3.4.11）
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "crypto_enums.h"

namespace zq {
namespace office {
namespace crypto {

/// 密码派生工具（全 static）
class PasswordDerivation {
public:
    /// XOR 混淆：派生密码验证子（[MS-OFFCRYPTO] 2.3.7.1）
    /// @param passwordUtf16 密码（UTF-16LE 字节流）
    /// @return 16 位验证子
    static std::uint16_t DeriveXorVerifier(const std::string& passwordUtf16);

    /// RC4 加密：派生基础密钥（[MS-OFFCRYPTO] 2.3.6.1）
    /// 算法：H = SHA-1(password_utf16 + salt)，取 H[0:keySize]
    /// @param passwordUtf16 密码（UTF-16LE 字节流）
    /// @param salt salt（16 字节）
    /// @param keySize 密钥长度（字节，5/10/16/20）
    /// @return RC4 密钥
    static std::string DeriveRC4Key(const std::string& passwordUtf16,
                                     const std::string& salt,
                                     std::uint32_t keySize);

    /// RC4 加密：派生分块密钥（[MS-OFFCRYPTO] 2.3.6.1）
    /// 算法：H_block = SHA-1(H0 + blockId + blockId)
    /// @param baseKey 基础密钥 H0（20 字节）
    /// @param blockId 分块 ID
    /// @param keySize 密钥长度
    /// @return 分块 RC4 密钥
    static std::string DeriveRC4BlockKey(const std::string& baseKey,
                                          std::uint32_t blockId,
                                          std::uint32_t keySize);

    /// Agile 加密：迭代 HMAC 派生（[MS-OFFCRYPTO] 2.3.4.9）
    /// 算法：
    ///   h1 = HMAC(hashAlg, password, salt)
    ///   h_n = HMAC(hashAlg, h_{n-1}, "")  // 迭代 spinCount - 1 次
    /// @param password 密码
    /// @param salt salt
    /// @param spinCount 迭代次数
    /// @param hashAlg 哈希算法
    /// @return 派生哈希值（hashSize 字节）
    static std::string DeriveAgileHash(const std::string& password,
                                        const std::string& salt,
                                        std::uint32_t spinCount,
                                        HashAlgorithm hashAlg);

    /// Agile 加密：blockKey 派生（[MS-OFFCRYPTO] 2.3.4.10）
    /// 算法：HMAC(hashAlg, hashValue, blockKey)
    /// @param hashValue 上一轮派生的哈希值
    /// @param blockKey blockKey（通常 8 字节固定值）
    /// @param hashAlg 哈希算法
    /// @return blockKey 派生值
    static std::string DeriveBlockKey(const std::string& hashValue,
                                       const std::string& blockKey,
                                       HashAlgorithm hashAlg);

    /// Agile 加密：分段密钥派生（[MS-OFFCRYPTO] 2.3.4.11）
    /// 算法：取 DeriveBlockKey 结果前 keySize 字节
    /// @param hashValue 上一轮派生的哈希值
    /// @param blockKey blockKey
    /// @param hashAlg 哈希算法
    /// @param keySize 密钥长度
    /// @return 分段密钥
    static std::string DeriveSegmentKey(const std::string& hashValue,
                                         const std::string& blockKey,
                                         HashAlgorithm hashAlg,
                                         std::uint32_t keySize);
};

} // namespace crypto
} // namespace office
} // namespace zq
