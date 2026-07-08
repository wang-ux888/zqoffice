// =============================================================================
// src/crypto/rc4_encryption.h / rc4_encryption.cpp
//
// RC4Encryption 类（Office 97-2003 通用 RC4 加密）
//
//   基于 [MS-OFFCRYPTO] 2.3.6 节实现。
//
//   算法流程：
//     1. Parse(stream)：从 Encryption stream 读取 Header + Verifier
//     2. VerifyPassword(password)：
//        a. H = SHA-1(password_utf16le + salt)  // 20 字节
//        b. RC4 key = H[0:keySize]
//        c. 用 RC4 key 解密 encryptedVerifier → verifier (16 字节)
//        d. 计算 SHA-1(verifier) → hash (20 字节)
//        e. 用 RC4 key 解密 encryptedVerifierHash → decryptedHash
//        f. 比对 hash 与 decryptedHash 前 20 字节
//     3. GenerateCipher(blockId)：
//        a. H_block = SHA-1(H0 + blockId + blockId)
//        b. RC4 key = H_block[0:keySize]
//        c. 创建 RC4 Cipher 并 Init(key)
//     4. DecryptPackageStream：使用分块 RC4 解密
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "encryption_header.h"
#include "encryption_verifier.h"
#include "i_encryption.h"

namespace zq {
namespace office {
namespace crypto {

class InputStream;

/// RC4 加密（Office 97-2003）
class RC4Encryption : public IEncryption {
public:
    RC4Encryption() = default;
    ~RC4Encryption() override = default;

    // ---------------------------------------------------------------------------
    // IEncryption 接口实现
    // ---------------------------------------------------------------------------
    void Parse(InputStream& stream) override;
    bool VerifyPassword(const std::string& password) override;
    void DecryptBlockedStream(std::string& stream, std::size_t blockSize) override;
    void ReportCryptAlgoInfo(NativeEventListener* /*listener*/) override {}
    std::unique_ptr<Cipher> GenerateCipher(std::uint32_t blockId) override;

    // ---------------------------------------------------------------------------
    // RC4 专属方法
    // ---------------------------------------------------------------------------

    /// 解密包流（一次性解密整个加密包）
    void DecryptPackageStream(std::string& stream);

    /// 取加密头
    const EncryptionHeader& GetHeader() const { return header_; }

    /// 取加密验证子
    const EncryptionVerifier& GetVerifier() const { return verifier_; }

    /// 取密码派生的基础密钥（H0，20 字节 SHA-1 摘要）
    const std::string& GetBaseKey() const { return baseKey_; }

    /// 是否已验证密码
    bool IsVerified() const { return verified_; }

private:
    EncryptionHeader  header_;
    EncryptionVerifier verifier_;
    std::string       baseKey_;   // H0 = SHA-1(password + salt)，20 字节
    bool              verified_ = false;
};

} // namespace crypto
} // namespace office
} // namespace zq
