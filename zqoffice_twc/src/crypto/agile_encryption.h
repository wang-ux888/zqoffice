// =============================================================================
// src/crypto/agile_encryption.h / agile_encryption.cpp
//
// AgileEncryption 类（Office 2007+ Agile 加密数据结构）
//
// 对应 EncryptionInfo XML 中的 <encryption> 根节点：
//   <encryption>
//     <keyData .../>
//     <dataIntegrity .../>
//     <keyEncryptors>
//       <keyEncryptor>
//         <p:encryptedKey .../>
//       </keyEncryptor>
//     </keyEncryptors>
//   </encryption>
//
//   [MS-OFFCRYPTO] 2.3.4 节定义。
//
//   注意：本类仅存储数据结构，密码验证和解密逻辑在 EncryptionInfo 中实现。
// =============================================================================
#pragma once

#include <memory>

#include "key_data.h"

namespace zq {
namespace office {
namespace crypto {

/// Agile 加密节点（<encryption> 根节点）
class AgileEncryption {
public:
    AgileEncryption() = default;
    ~AgileEncryption() = default;

    // 子节点
    KeyData            keyData;            // <keyData>
    DataIntegrity      dataIntegrity;      // <dataIntegrity>
    PasswordEncryptor  passwordEncryptor;  // <p:encryptedKey>

    // 派生数据（VerifyPassword 后填充）
    std::string verifierHashInput;   // 解密后的验证子哈希输入
    std::string verifierHashValue;   // 解密后的验证子哈希值
    std::string secretKey;           // 解密后的 keyData 密钥（用于解密 package）
    std::string hmacKey;             // 解密后的 HMAC 密钥
    bool verified = false;

    /// 是否已验证
    bool IsVerified() const { return verified; }
};

} // namespace crypto
} // namespace office
} // namespace zq
