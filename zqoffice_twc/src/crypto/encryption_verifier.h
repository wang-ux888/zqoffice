// =============================================================================
// src/crypto/encryption_verifier.h
//
// EncryptionVerifier 结构体（加密验证子）
//
//   存储密码验证所需数据（salt + encryptedVerifier + encryptedVerifierHash）。
//   [MS-OFFCRYPTO] 2.3.6.2 / 2.3.4.3 节定义。
//
//   字段：
//     - saltSize: salt 长度（字节，通常 16）
//     - salt: salt 值
//     - encryptedVerifier: 加密的验证子（用于校验密码）
//     - verifierHashSize: 验证子哈希长度
//     - encryptedVerifierHash: 加密的验证子哈希
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace zq {
namespace office {
namespace crypto {

class InputStream;

/// 加密验证子
struct EncryptionVerifier {
    std::uint32_t saltSize = 0;          // salt 长度（字节）
    std::string   salt;                  // salt（16 字节）
    std::string   encryptedVerifier;     // 加密的验证子（16 字节）
    std::uint32_t verifierHashSize = 0;  // 验证子哈希长度（字节）
    std::string   encryptedVerifierHash;  // 加密的验证子哈希（32 字节，SHA-1 摘要）

    /// 从输入流解析（[MS-OFFCRYPTO] 2.3.6.2 节）
    void Parse(InputStream& stream);

    // 默认/移动/拷贝构造与赋值
    EncryptionVerifier() = default;
    EncryptionVerifier(EncryptionVerifier&&) = default;
    EncryptionVerifier(const EncryptionVerifier&) = default;
    EncryptionVerifier& operator=(EncryptionVerifier&&) = default;
    EncryptionVerifier& operator=(const EncryptionVerifier&) = default;
};

} // namespace crypto
} // namespace office
} // namespace zq
