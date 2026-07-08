// =============================================================================
// src/crypto/agile_encryption_handler.h / agile_encryption_handler.cpp
//
// AgileEncryptionHandler / KeyDataHandler（SAX Handler）
//
//   解析 OOXML Agile 加密的 EncryptionInfo XML。
//
//   XML 结构：
//   <encryption>
//     <keyData saltSize="16" blockSize="16" keyBits="256" hashSize="64"
//              cipherAlgorithm="AES" cipherChaining="ChainingModeCBC"
//              hashAlgorithm="SHA512" saltValue="base64..."/>
//     <dataIntegrity encryptedHmacKey="base64..." encryptedHmacValue="base64..."/>
//     <keyEncryptors>
//       <keyEncryptor uri="http://schemas.microsoft.com/office/2006/keyEncryptor/password">
//         <p:encryptedKey spinCount="100000" saltSize="16" blockSize="16" keyBits="256"
//                         hashSize="64" cipherAlgorithm="AES" cipherChaining="ChainingModeCBC"
//                         hashAlgorithm="SHA512"
//                         saltValue="base64..."
//                         encryptedVerifierHashInput="base64..."
//                         encryptedVerifierHashValue="base64..."
//                         encryptedKeyValue="base64..."/>
//       </keyEncryptor>
//     </keyEncryptors>
//   </encryption>
//
//   实现：使用 pugixml DOM 解析（pugixml 已在 Phase C 引入），按 localName 匹配。
// =============================================================================
#pragma once

#include <memory>
#include <string>

#include "agile_encryption.h"
#include "key_data.h"

namespace zq {
namespace office {
namespace crypto {

/// Agile 加密 SAX Handler（解析 EncryptionInfo XML）
class AgileEncryptionHandler {
public:
    AgileEncryptionHandler() = default;
    virtual ~AgileEncryptionHandler() = default;

    virtual void OnStartElement(const std::string& name);
    virtual void OnCharacterDataHandler(const std::string& data);
    virtual void OnEndElement(const std::string& name);

    /// 取结果节点
    std::unique_ptr<AgileEncryption> GetNode();

    /// 直接解析 XML 字符串（便捷方法）
    /// @param xml XML 内容
    /// @return 是否成功
    bool ParseXml(const std::string& xml);

private:
    std::unique_ptr<AgileEncryption> node_;
    std::string currentElement_;
    std::string currentText_;
};

/// KeyData SAX Handler（解析 <keyData> 子元素）
class KeyDataHandler {
public:
    KeyDataHandler() = default;
    virtual ~KeyDataHandler() = default;

    virtual void OnStartElement(const std::string& name);
    virtual void OnCharacterDataHandler(const std::string& data);
    virtual void OnEndElement(const std::string& name);

    /// 取结果
    std::unique_ptr<KeyData> GetNode();

private:
    std::unique_ptr<KeyData> node_;
    std::string currentElement_;
    std::string currentText_;
};

} // namespace crypto
} // namespace office
} // namespace zq
