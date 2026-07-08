// =============================================================================
// src/crypto/encryption_info.h / encryption_info.cpp
//
// EncryptionInfo 类（Office 2007+ Agile 加密入口）
//
//   继承 IEncryption，实现 Agile 加密的密码验证 + 解密。
//
//   算法流程（[MS-OFFCRYPTO] 2.3.4.9-2.3.4.11）：
//     1. Parse(stream)：解析 EncryptionInfo XML（通过 AgileEncryptionHandler SAX）
//     2. VerifyPassword(password)：
//        a. h1 = HMAC(password, saltValue) using hashAlgorithm
//        b. h_n = HMAC(h_{n-1}, blockKey) 迭代 spinCount 次
//        c. 派生 verifierHashInput key、verifierHashValue key、secretKey key
//        d. 解密 encryptedVerifierHashInput / encryptedVerifierHashValue / encryptedKeyValue
//        e. 验证：SHA-(hashAlg)(verifierHashInput) == verifierHashValue
//     3. GenerateCipher(blockId)：生成 AES-CBC Cipher
//        a. blockKey = SHA-512(blockId) 前 hashSize 字节
//        b. iv = HMAC(secretKey, blockKey) ... 复杂流程
//        c. 创建 AES Cipher 并 Init(key, iv, CBC)
//     4. DecryptBlockedStream：分块 AES-CBC 解密
// =============================================================================
#pragma once

#include <memory>
#include <string>

#include "agile_encryption.h"
#include "i_encryption.h"

namespace zq {
namespace office {
namespace crypto {

class InputStream;

/// Agile 加密信息（继承 IEncryption）
class EncryptionInfo : public IEncryption {
public:
    EncryptionInfo() = default;
    ~EncryptionInfo() override = default;

    // ---------------------------------------------------------------------------
    // IEncryption 接口实现
    // ---------------------------------------------------------------------------
    void Parse(InputStream& stream) override;
    bool VerifyPassword(const std::string& password) override;
    void DecryptBlockedStream(std::string& stream, std::size_t blockSize) override;
    void ReportCryptAlgoInfo(NativeEventListener* /*listener*/) override {}
    std::unique_ptr<Cipher> GenerateCipher(std::uint32_t blockId) override;

    // ---------------------------------------------------------------------------
    // Agile 专属方法
    // ---------------------------------------------------------------------------

    /// 取 Agile 节点
    std::unique_ptr<AgileEncryption> GetNode();

    /// 直接设置 Agile 节点（用于 SAX Handler 解析后注入）
    void SetNode(std::unique_ptr<AgileEncryption> node);

    /// 是否已验证
    bool IsVerified() const { return verified_; }

    /// 取 Agile 节点引用（const）
    const AgileEncryption* GetAgileNode() const { return agile_.get(); }

private:
    /// Agile 加密节点数据
    std::unique_ptr<AgileEncryption> agile_;

    /// 密码验证后派生的总密钥（用于解密 package）
    std::string secretKey_;

    bool verified_ = false;

    // ---------------------------------------------------------------------------
    // 内部辅助函数
    // ---------------------------------------------------------------------------

    /// 派生 hash 值：HMAC(password, salt, hashAlg)
    std::string computeHash(const std::string& password,
                             const std::string& salt,
                             HashAlgorithm hashAlg);

    /// 派生 blockKey：HMAC(hashValue, blockKey, hashAlg)
    std::string computeBlockKey(const std::string& hashValue,
                                 const std::string& blockKey,
                                 HashAlgorithm hashAlg);

    /// AES-CBC 解密辅助
    /// @param data 输入数据（in-place 解密）
    /// @param key AES 密钥
    /// @param iv 初始向量（16 字节）
    /// @return 成功/失败
    bool decryptAesCbc(std::string& data, const std::string& key,
                       const std::string& iv);
};

} // namespace crypto
} // namespace office
} // namespace zq
