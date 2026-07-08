// =============================================================================
// src/crypto/encryption_header.h
//
// EncryptionHeader 结构体（加密头信息）
//
//   描述加密算法、哈希算法、密钥长度等元数据。
//   Office 97-2003 RC4 加密和 Office 2007+ Agile 加密共用此结构。
//
//   [MS-OFFCRYPTO] 2.3.6.1 / 2.3.4.2 节定义。
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>

#include "crypto_enums.h"
#include "cipher.h"
#include "message_digest.h"

namespace zq {
namespace office {
namespace crypto {

class InputStream;

/// 加密头
struct EncryptionHeader {
    // 算法字段
    CipherAlgorithm cipherAlgorithm = CipherAlgorithm::kUnknown;
    CipherChaining  cipherChaining  = CipherChaining::kUnknown;
    HashAlgorithm   hashAlgorithm   = HashAlgorithm::kUnknown;

    // 密钥长度（字节，RC4 通常 5/10/16，AES 通常 16/24/32）
    std::uint32_t keySize = 0;

    // 块大小（字节，AES=16，RC4=1）
    std::uint32_t blockSize = 0;

    // 哈希大小（字节，SHA-1=20，SHA-512=64）
    std::uint32_t hashSize = 0;

    // 算法 ID（CryptographyAlgorithmId，[MS-OFFCRYPTO] 2.3.6.1）
    // 0x0000660E = AES, 0x00006802 = RC4, 0x00006603 = 3DES
    std::uint32_t cipherAlgorithmId = 0;

    // 哈希算法 ID
    // 0x00008004 = SHA-1, 0x0000800C = SHA-256, 0x0000800E = SHA-512
    std::uint32_t hashAlgorithmId = 0;

    /// 取加密算法（解析 cipherAlgorithmId）
    CipherAlgorithm GetCipherAlgorithm() const {
        if (cipherAlgorithm != CipherAlgorithm::kUnknown) return cipherAlgorithm;
        // 从 ID 解析
        switch (cipherAlgorithmId) {
            case 0x0000660E: return CipherAlgorithm::kAES;
            case 0x00006802: return CipherAlgorithm::kRC4;
            case 0x00006603: return CipherAlgorithm::k3DES;
            default:         return CipherAlgorithm::kUnknown;
        }
    }

    /// 取哈希算法（解析 hashAlgorithmId）
    HashAlgorithm GetHashAlgorithm() const {
        if (hashAlgorithm != HashAlgorithm::kUnknown) return hashAlgorithm;
        switch (hashAlgorithmId) {
            case 0x00008004: return HashAlgorithm::kSHA1;
            case 0x0000800C: return HashAlgorithm::kSHA256;
            case 0x0000800E: return HashAlgorithm::kSHA512;
            default:         return HashAlgorithm::kUnknown;
        }
    }

    /// 创建对应的 Cipher
    std::unique_ptr<Cipher> CreateCipher() const;

    /// 创建对应的 MessageDigest
    std::unique_ptr<MessageDigest> CreateMessageDigest() const;

    // 默认/移动/拷贝构造与赋值
    EncryptionHeader() = default;
    EncryptionHeader(EncryptionHeader&&) = default;
    EncryptionHeader(const EncryptionHeader&) = default;
    EncryptionHeader& operator=(EncryptionHeader&&) = default;
    EncryptionHeader& operator=(const EncryptionHeader&) = default;
};

} // namespace crypto
} // namespace office
} // namespace zq
