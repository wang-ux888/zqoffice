// =============================================================================
// src/crypto/agile_encryption_handler.cpp
//
// AgileEncryptionHandler / KeyDataHandler 实现
// =============================================================================
#include "agile_encryption_handler.h"

#include <cstring>

#include <pugixml.hpp>

#include "internal/base64.h"
#include "crypto_enums.h"

namespace zq {
namespace office {
namespace crypto {

namespace {

/// 取元素的 localName（去除命名空间前缀，如 "p:encryptedKey" → "encryptedKey"）
std::string localName(const std::string& name) {
    auto pos = name.find(':');
    return (pos == std::string::npos) ? name : name.substr(pos + 1);
}

/// 取属性值（按 localName 匹配）
std::string getAttr(const pugi::xml_node& node, const std::string& localName) {
    for (auto attr = node.first_attribute(); attr; attr = attr.next_attribute()) {
        std::string name = attr.name();
        auto pos = name.find(':');
        std::string ln = (pos == std::string::npos) ? name : name.substr(pos + 1);
        if (ln == localName) {
            return attr.value();
        }
    }
    return std::string();
}

/// 从属性填充 KeyData
void fillKeyDataFromAttributes(KeyData& kd, const pugi::xml_node& node) {
    std::string v;
    v = getAttr(node, "saltSize");
    if (!v.empty()) kd.saltSize = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "blockSize");
    if (!v.empty()) kd.blockSize = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "keyBits");
    if (!v.empty()) kd.keyBits = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "hashSize");
    if (!v.empty()) kd.hashSize = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "cipherAlgorithm");
    kd.cipherAlgorithmName = v;
    if (!v.empty()) kd.cipherAlgorithm = parseCipherAlgorithm(v);
    v = getAttr(node, "cipherChaining");
    kd.cipherChainingName = v;
    if (!v.empty()) kd.cipherChaining = parseCipherChaining(v);
    v = getAttr(node, "hashAlgorithm");
    kd.hashAlgorithmName = v;
    if (!v.empty()) kd.hashAlgorithm = parseHashAlgorithm(v);
    v = getAttr(node, "saltValue");
    if (!v.empty()) kd.saltValue = internal::base64Decode(v);
}

/// 从属性填充 PasswordEncryptor
void fillPasswordEncryptorFromAttributes(PasswordEncryptor& pe, const pugi::xml_node& node) {
    std::string v;
    v = getAttr(node, "saltSize");
    if (!v.empty()) pe.saltSize = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "blockSize");
    if (!v.empty()) pe.blockSize = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "keyBits");
    if (!v.empty()) pe.keyBits = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "hashSize");
    if (!v.empty()) pe.hashSize = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "spinCount");
    if (!v.empty()) pe.spinCount = static_cast<std::uint32_t>(std::stoul(v));
    v = getAttr(node, "cipherAlgorithm");
    pe.cipherAlgorithmName = v;
    if (!v.empty()) pe.cipherAlgorithm = parseCipherAlgorithm(v);
    v = getAttr(node, "cipherChaining");
    pe.cipherChainingName = v;
    if (!v.empty()) pe.cipherChaining = parseCipherChaining(v);
    v = getAttr(node, "hashAlgorithm");
    pe.hashAlgorithmName = v;
    if (!v.empty()) pe.hashAlgorithm = parseHashAlgorithm(v);
    v = getAttr(node, "saltValue");
    if (!v.empty()) pe.saltValue = internal::base64Decode(v);
    v = getAttr(node, "encryptedVerifierHashInput");
    if (!v.empty()) pe.encryptedVerifierHashInput = internal::base64Decode(v);
    v = getAttr(node, "encryptedVerifierHashValue");
    if (!v.empty()) pe.encryptedVerifierHashValue = internal::base64Decode(v);
    v = getAttr(node, "encryptedKeyValue");
    if (!v.empty()) pe.encryptedKeyValue = internal::base64Decode(v);
}

} // namespace

// ===========================================================================
// AgileEncryptionHandler
// ===========================================================================
void AgileEncryptionHandler::OnStartElement(const std::string& name) {
    currentElement_ = localName(name);
    currentText_.clear();
}

void AgileEncryptionHandler::OnCharacterDataHandler(const std::string& data) {
    currentText_ += data;
}

void AgileEncryptionHandler::OnEndElement(const std::string& /*name*/) {
    currentElement_.clear();
    currentText_.clear();
}

std::unique_ptr<AgileEncryption> AgileEncryptionHandler::GetNode() {
    return std::move(node_);
}

bool AgileEncryptionHandler::ParseXml(const std::string& xml) {
    pugi::xml_document doc;
    auto result = doc.load_string(xml.c_str());
    if (!result) {
        return false;
    }

    node_ = std::make_unique<AgileEncryption>();

    // 查找 <encryption> 根节点（namespace 可能是 p: 或无前缀）
    pugi::xml_node encryptionNode;
    for (auto child : doc.children()) {
        if (localName(child.name()) == "encryption") {
            encryptionNode = child;
            break;
        }
    }
    if (!encryptionNode) return false;

    // 遍历子节点
    for (auto child : encryptionNode.children()) {
        std::string ln = localName(child.name());

        if (ln == "keyData") {
            fillKeyDataFromAttributes(node_->keyData, child);
        } else if (ln == "dataIntegrity") {
            std::string v;
            v = getAttr(child, "encryptedHmacKey");
            if (!v.empty()) node_->dataIntegrity.encryptedHmacKey = internal::base64Decode(v);
            v = getAttr(child, "encryptedHmacValue");
            if (!v.empty()) node_->dataIntegrity.encryptedHmacValue = internal::base64Decode(v);
        } else if (ln == "keyEncryptors") {
            // 遍历 <keyEncryptor>
            for (auto ke : child.children()) {
                if (localName(ke.name()) != "keyEncryptor") continue;
                // 遍历 <p:encryptedKey>
                for (auto ek : ke.children()) {
                    if (localName(ek.name()) == "encryptedKey") {
                        fillPasswordEncryptorFromAttributes(node_->passwordEncryptor, ek);
                    }
                }
            }
        }
    }

    return true;
}

// ===========================================================================
// KeyDataHandler
// ===========================================================================
void KeyDataHandler::OnStartElement(const std::string& name) {
    currentElement_ = localName(name);
    currentText_.clear();
}

void KeyDataHandler::OnCharacterDataHandler(const std::string& data) {
    currentText_ += data;
}

void KeyDataHandler::OnEndElement(const std::string& /*name*/) {
    currentElement_.clear();
    currentText_.clear();
}

std::unique_ptr<KeyData> KeyDataHandler::GetNode() {
    return std::move(node_);
}

} // namespace crypto
} // namespace office
} // namespace zq
