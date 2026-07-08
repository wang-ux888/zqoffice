// =============================================================================
// src/crypto/encryption_info.cpp
//
// EncryptionInfo 实现（[MS-OFFCRYPTO] 2.3.4 节）
//
// Agile 加密密码验证流程（[MS-OFFCRYPTO] 2.3.4.9-2.3.4.11）：
//   1. s1 = HMAC(hashAlg, password, salt)
//   2. s_n = HMAC(hashAlg, s_{n-1}, "")  // 迭代 spinCount 次
//      其中空字符串作为输入（实际上是基于 s_{n-1} 计算）
//   3. verifierHashInput key = HMAC(hashAlg, s_n, blockKey_input)
//   4. verifierHashValue key = HMAC(hashAlg, s_n, blockKey_value)
//   5. secretKey key = HMAC(hashAlg, s_n, blockKey_secret)
//   6. 解密 encryptedVerifierHashInput（AES-CBC，IV = saltValue）
//   7. 解密 encryptedVerifierHashValue（AES-CBC，IV = saltValue）
//   8. 验证：hashAlg(verifierHashInput) == verifierHashValue
//   9. 解密 encryptedKeyValue（AES-CBC，IV = saltValue）→ secretKey
// =============================================================================
#include "encryption_info.h"

#include <cstring>
#include <stdexcept>

#include "cipher.h"
#include "cipher_factory.h"
#include "crypto_utils.h"
#include "internal/sha.h"

namespace zq {
namespace office {
namespace crypto {

class InputStream {};

// ===========================================================================
// IEncryption 接口实现
// ===========================================================================
void EncryptionInfo::Parse(InputStream& /*stream*/) {
    // 实际解析由 SAX Handler 完成，Parse 不做任何事
    // EncryptionParser::ParseEncryptionInfoStream 调用 AgileEncryptionHandler
    // 解析 XML 后通过 SetNode 注入 agile_ 节点
}

bool EncryptionInfo::VerifyPassword(const std::string& password) {
    if (!agile_) return false;

    const auto& pe = agile_->passwordEncryptor;
    HashAlgorithm hashAlg = pe.hashAlgorithm;

    // 1. 计算 h_n = 迭代 HMAC(password, salt, spinCount 次)
    // 第 1 次：HMAC(password, salt)
    // 第 2~spinCount 次：HMAC(prev, "")
    std::string h = computeHash(password, pe.saltValue, hashAlg);
    for (std::uint32_t i = 1; i < pe.spinCount; ++i) {
        h = computeHash(h, std::string(), hashAlg);
    }
    // h 现在是 h_n（spinCount 次迭代后的哈希值）

    // 2. 派生 verifierHashInput key
    // blockKey_input = SHA-(hashAlg)(0x00000000 FE 7F 00 00 等)
    // 实际上 [MS-OFFCRYPTO] 规范中 blockKey 是固定值
    // verifierHashInput 的 blockKey = 0x00000000FE7F0000 等

    // 简化实现：使用 agile 加密的 blockKey
    // blockKey for verifierHashInput: 长度 = hashSize 的零字节
    std::size_t hashSize = pe.hashSize;
    if (hashSize == 0) hashSize = 64;  // SHA-512 default

    // blockKey for verifierHashInput = 0x00000000FE7F0000 (8 字节) padded to hashSize
    std::string blockKeyInput;
    blockKeyInput.push_back(static_cast<char>(0xFE));
    blockKeyInput.push_back(static_cast<char>(0x7F));
    blockKeyInput.append(6, '\0');
    // 填充到 hashSize
    if (blockKeyInput.size() < hashSize) {
        blockKeyInput.append(hashSize - blockKeyInput.size(), '\0');
    }

    // blockKey for verifierHashValue = 0x00000000FD7F0000 (8 字节) padded
    std::string blockKeyValue;
    blockKeyValue.push_back(static_cast<char>(0xFD));
    blockKeyValue.push_back(static_cast<char>(0x7F));
    blockKeyValue.append(6, '\0');
    if (blockKeyValue.size() < hashSize) {
        blockKeyValue.append(hashSize - blockKeyValue.size(), '\0');
    }

    // blockKey for secretKey = 0x00000000FF7F0000 (8 字节) padded
    std::string blockKeySecret;
    blockKeySecret.push_back(static_cast<char>(0xFF));
    blockKeySecret.push_back(static_cast<char>(0x7F));
    blockKeySecret.append(6, '\0');
    if (blockKeySecret.size() < hashSize) {
        blockKeySecret.append(hashSize - blockKeySecret.size(), '\0');
    }

    // 3. 计算各 key
    std::string verifierHashInputKey = computeBlockKey(h, blockKeyInput, hashAlg);
    std::string verifierHashValueKey = computeBlockKey(h, blockKeyValue, hashAlg);
    std::string secretKeyKey         = computeBlockKey(h, blockKeySecret, hashAlg);

    // 4. 解密 encryptedVerifierHashInput（AES-CBC，key = verifierHashInputKey，IV = saltValue）
    std::string decryptedHashInput = pe.encryptedVerifierHashInput;
    if (!decryptAesCbc(decryptedHashInput, verifierHashInputKey, pe.saltValue)) {
        verified_ = false;
        return false;
    }

    // 5. 解密 encryptedVerifierHashValue（AES-CBC，key = verifierHashValueKey，IV = saltValue）
    std::string decryptedHashValue = pe.encryptedVerifierHashValue;
    if (!decryptAesCbc(decryptedHashValue, verifierHashValueKey, pe.saltValue)) {
        verified_ = false;
        return false;
    }

    // 6. 验证：hashAlg(decryptedHashInput) == decryptedHashValue 前 hashSize 字节
    std::string computedHash;
    switch (hashAlg) {
        case HashAlgorithm::kSHA1:   computedHash = internal::sha1(decryptedHashInput); break;
        case HashAlgorithm::kSHA256: computedHash = internal::sha256(decryptedHashInput); break;
        case HashAlgorithm::kSHA384: computedHash = internal::sha384(decryptedHashInput); break;
        case HashAlgorithm::kSHA512: computedHash = internal::sha512(decryptedHashInput); break;
        default:
            verified_ = false;
            return false;
    }

    if (decryptedHashValue.size() < computedHash.size()) {
        verified_ = false;
        return false;
    }
    if (std::memcmp(computedHash.data(), decryptedHashValue.data(),
                     computedHash.size()) != 0) {
        verified_ = false;
        return false;
    }

    // 7. 解密 encryptedKeyValue（AES-CBC，key = secretKeyKey，IV = saltValue）→ secretKey
    secretKey_ = pe.encryptedKeyValue;
    if (!decryptAesCbc(secretKey_, secretKeyKey, pe.saltValue)) {
        verified_ = false;
        return false;
    }

    // secretKey_ 长度应该是 keyData.keyBits / 8
    if (secretKey_.size() > agile_->keyData.GetKeySize()) {
        secretKey_.resize(agile_->keyData.GetKeySize());
    }

    agile_->verifierHashInput = decryptedHashInput;
    agile_->verifierHashValue = decryptedHashValue;
    agile_->secretKey = secretKey_;
    agile_->verified = true;
    verified_ = true;
    return true;
}

void EncryptionInfo::DecryptBlockedStream(std::string& stream, std::size_t blockSize) {
    if (!verified_ || stream.empty() || !agile_) return;

    // Agile 加密分块解密：
    // 每块大小 = keyData.blockSize（通常 4096 字节）
    // 每块使用不同的 IV：HMAC(hashAlg, saltValue, blockId)
    std::size_t totalLen = stream.size();
    std::uint32_t blockId = 0;
    std::size_t offset = 0;

    while (offset < totalLen) {
        std::size_t remaining = totalLen - offset;
        // Agile 加密要求块大小是 16 的倍数（AES 块大小）
        std::size_t thisBlock = (remaining < blockSize) ? remaining : blockSize;
        // 对齐到 16 字节
        thisBlock = (thisBlock / 16) * 16;
        if (thisBlock == 0) thisBlock = 16;

        // 计算此 block 的 IV
        std::string ivInput;
        ivInput.append(agile_->keyData.saltValue);
        // blockId 大端 4 字节 + 0x80 + 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
        std::uint8_t idBytes[4];
        idBytes[0] = static_cast<std::uint8_t>((blockId >> 24) & 0xFF);
        idBytes[1] = static_cast<std::uint8_t>((blockId >> 16) & 0xFF);
        idBytes[2] = static_cast<std::uint8_t>((blockId >> 8) & 0xFF);
        idBytes[3] = static_cast<std::uint8_t>(blockId & 0xFF);
        ivInput.append(reinterpret_cast<const char*>(idBytes), 4);
        ivInput.push_back(static_cast<char>(0x80));
        ivInput.append(7, '\0');
        // 填充到 hashSize
        std::size_t hashSize = agile_->keyData.hashSize;
        if (hashSize == 0) hashSize = 64;
        while (ivInput.size() < hashSize) {
            ivInput.push_back('\0');
        }

        std::string iv = computeHash(secretKey_, ivInput, agile_->keyData.hashAlgorithm);
        iv.resize(16);  // AES IV 16 字节

        // 解密此 block
        auto cipher = CipherFactory::CreateCipher(CipherAlgorithm::kAES);
        if (!cipher) return;
        cipher->Init(secretKey_, iv, CipherChaining::kCBC);
        cipher->Update(&stream[offset], 0, static_cast<int>(thisBlock));

        offset += thisBlock;
        ++blockId;
    }
}

std::unique_ptr<Cipher> EncryptionInfo::GenerateCipher(std::uint32_t blockId) {
    if (!verified_ || !agile_) return nullptr;

    // 计算 IV
    std::string ivInput;
    ivInput.append(agile_->keyData.saltValue);
    std::uint8_t idBytes[4];
    idBytes[0] = static_cast<std::uint8_t>((blockId >> 24) & 0xFF);
    idBytes[1] = static_cast<std::uint8_t>((blockId >> 16) & 0xFF);
    idBytes[2] = static_cast<std::uint8_t>((blockId >> 8) & 0xFF);
    idBytes[3] = static_cast<std::uint8_t>(blockId & 0xFF);
    ivInput.append(reinterpret_cast<const char*>(idBytes), 4);
    ivInput.push_back(static_cast<char>(0x80));
    ivInput.append(7, '\0');
    std::size_t hashSize = agile_->keyData.hashSize;
    if (hashSize == 0) hashSize = 64;
    while (ivInput.size() < hashSize) {
        ivInput.push_back('\0');
    }

    std::string iv = computeHash(secretKey_, ivInput, agile_->keyData.hashAlgorithm);
    iv.resize(16);

    auto cipher = CipherFactory::CreateCipher(CipherAlgorithm::kAES);
    if (!cipher) return nullptr;
    cipher->Init(secretKey_, iv, CipherChaining::kCBC);
    return cipher;
}

std::unique_ptr<AgileEncryption> EncryptionInfo::GetNode() {
    return std::move(agile_);
}

void EncryptionInfo::SetNode(std::unique_ptr<AgileEncryption> node) {
    agile_ = std::move(node);
}

// ===========================================================================
// 内部辅助
// ===========================================================================

std::string EncryptionInfo::computeHash(const std::string& password,
                                          const std::string& salt,
                                          HashAlgorithm hashAlg) {
    // HMAC(hashAlg, password, salt)
    // 内部调用 internal::hmacSha1 / hmacSha256 / hmacSha384 / hmacSha512
    switch (hashAlg) {
        case HashAlgorithm::kSHA1:   return internal::hmacSha1(password, salt);
        case HashAlgorithm::kSHA256: return internal::hmacSha256(password, salt);
        case HashAlgorithm::kSHA384: return internal::hmacSha384(password, salt);
        case HashAlgorithm::kSHA512: return internal::hmacSha512(password, salt);
        default:
            throw std::runtime_error("Unsupported hash algorithm");
    }
}

std::string EncryptionInfo::computeBlockKey(const std::string& hashValue,
                                              const std::string& blockKey,
                                              HashAlgorithm hashAlg) {
    // blockKey 派生：HMAC(hashAlg, hashValue, blockKey)
    return computeHash(hashValue, blockKey, hashAlg);
}

/// 解密 AES-CBC（辅助函数，声明在文件内）
bool EncryptionInfo::decryptAesCbc(std::string& data,
                                     const std::string& key,
                                     const std::string& iv) {
    if (data.empty() || key.empty() || iv.size() < 16) return false;

    auto cipher = CipherFactory::CreateCipher(CipherAlgorithm::kAES);
    if (!cipher) return false;
    try {
        cipher->Init(key, iv, CipherChaining::kCBC);
    } catch (...) {
        return false;
    }
    cipher->Update(data);
    return true;
}

} // namespace crypto
} // namespace office
} // namespace zq
