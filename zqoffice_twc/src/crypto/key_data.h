// =============================================================================
// src/crypto/key_data.h
//
// KeyData 结构体（Agile 加密密钥数据）
//
// 对应 OOXML Agile 加密 EncryptionInfo XML 中的 <keyData> 元素：
//   <keyData saltSize="16" blockSize="16" keyBits="256" hashSize="64"
//            cipherAlgorithm="AES" cipherChaining="ChainingModeCBC"
//            hashAlgorithm="SHA512" saltValue="base64..."/>
//
//   [MS-OFFCRYPTO] 2.3.4.6 节定义。
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "crypto_enums.h"

namespace zq {
namespace office {
namespace crypto {

/// Agile 加密密钥数据
struct KeyData {
    std::uint32_t saltSize = 0;        // salt 长度（字节）
    std::uint32_t blockSize = 16;      // 块大小（字节，AES=16）
    std::uint32_t keyBits = 0;         // 密钥位数（128/192/256）
    std::uint32_t hashSize = 0;        // 哈希长度（字节）

    // 算法
    CipherAlgorithm cipherAlgorithm = CipherAlgorithm::kUnknown;
    CipherChaining  cipherChaining  = CipherChaining::kUnknown;
    HashAlgorithm   hashAlgorithm   = HashAlgorithm::kUnknown;

    // 原始算法名（XML 中的字符串）
    std::string cipherAlgorithmName;
    std::string cipherChainingName;
    std::string hashAlgorithmName;

    // Salt 值（二进制）
    std::string saltValue;

    /// 密钥长度（字节） = keyBits / 8
    std::uint32_t GetKeySize() const { return keyBits / 8; }
};

/// 密码加密器（<p:encryptedKey> 元素）
struct PasswordEncryptor {
    std::uint32_t saltSize = 0;
    std::uint32_t blockSize = 16;
    std::uint32_t keyBits = 0;
    std::uint32_t hashSize = 0;
    std::uint32_t spinCount = 0;       // 迭代次数（通常 100000）

    CipherAlgorithm cipherAlgorithm = CipherAlgorithm::kUnknown;
    CipherChaining  cipherChaining  = CipherChaining::kUnknown;
    HashAlgorithm   hashAlgorithm   = HashAlgorithm::kUnknown;

    std::string cipherAlgorithmName;
    std::string cipherChainingName;
    std::string hashAlgorithmName;

    std::string saltValue;             // salt
    std::string encryptedVerifierHashInput;  // 加密的验证子哈希输入（16 字节）
    std::string encryptedVerifierHashValue;  // 加密的验证子哈希值（32+ 字节）
    std::string encryptedKeyValue;     // 加密的密钥值（用于解密 keyData）

    /// 密钥长度（字节）
    std::uint32_t GetKeySize() const { return keyBits / 8; }
};

/// 数据完整性（<dataIntegrity> 元素）
struct DataIntegrity {
    std::string encryptedHmacKey;   // 加密的 HMAC 密钥
    std::string encryptedHmacValue;  // 加密的 HMAC 值
};

} // namespace crypto
} // namespace office
} // namespace zq
