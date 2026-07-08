// =============================================================================
// src/crypto/rc4_encryption.cpp
//
// RC4Encryption 实现（[MS-OFFCRYPTO] 2.3.6 节）
// =============================================================================
#include "rc4_encryption.h"

#include <cstring>
#include <stdexcept>

#include "cipher.h"
#include "cipher_factory.h"
#include "crypto_utils.h"
#include "internal/sha.h"

namespace zq {
namespace office {
namespace crypto {

// InputStream 占位（实际实现由 EncryptionParser 处理）
class InputStream {};

// ===========================================================================
// IEncryption 接口实现
// ===========================================================================
void RC4Encryption::Parse(InputStream& /*stream*/) {
    // 实际解析由 EncryptionParser::ParseEncryption03 完成
    // 这里 Parse 是空实现（接口要求）
}

bool RC4Encryption::VerifyPassword(const std::string& password) {
    if (verifier_.salt.empty() || verifier_.encryptedVerifier.empty() ||
        verifier_.encryptedVerifierHash.empty()) {
        return false;
    }

    // 1. 派生基础密钥 H0 = SHA-1(password_utf16le + salt)
    std::string pwdUtf16 = CryptoUtils::Utf8ToUtf16(password);
    std::string input = pwdUtf16 + verifier_.salt;
    baseKey_ = internal::sha1(input);
    // baseKey_ 应为 20 字节

    // 2. 取 RC4 密钥 = H0[0:keySize]
    std::uint32_t keySize = header_.keySize;
    if (keySize == 0 || keySize > 20) keySize = 20;  // SHA-1 输出 20 字节
    std::string rc4Key = baseKey_.substr(0, keySize);

    // 3. 用 RC4 密钥解密 encryptedVerifier（16 字节）
    auto cipher = CipherFactory::CreateCipher(CipherAlgorithm::kRC4);
    if (!cipher) return false;
    cipher->Init(rc4Key, std::string(), CipherChaining::kUnknown);
    std::string decryptedVerifier = verifier_.encryptedVerifier;
    cipher->Update(decryptedVerifier);

    // 4. 计算 SHA-1(decryptedVerifier) → hash (20 字节)
    std::string expectedHash = internal::sha1(decryptedVerifier);

    // 5. 用同一 RC4 密钥解密 encryptedVerifierHash
    // 注意：对于 RC4，需要重新创建 Cipher（流密码状态不能复用）
    auto cipher2 = CipherFactory::CreateCipher(CipherAlgorithm::kRC4);
    if (!cipher2) return false;
    cipher2->Init(rc4Key, std::string(), CipherChaining::kUnknown);
    std::string decryptedHash = verifier_.encryptedVerifierHash;
    cipher2->Update(decryptedHash);

    // 6. 比对 hash 与 decryptedHash 前 20 字节
    if (decryptedHash.size() < 20) {
        verified_ = false;
        return false;
    }
    if (std::memcmp(expectedHash.data(), decryptedHash.data(), 20) != 0) {
        verified_ = false;
        return false;
    }

    verified_ = true;
    return true;
}

void RC4Encryption::DecryptBlockedStream(std::string& stream, std::size_t blockSize) {
    if (!verified_ || stream.empty()) return;

    // RC4 分块解密：每个 block（通常 1024 字节）使用独立的 RC4 Cipher
    // 每块的密钥 = SHA-1(H0 + blockId + blockId)[0:keySize]
    std::uint32_t keySize = header_.keySize;
    if (keySize == 0 || keySize > 20) keySize = 20;

    std::size_t totalLen = stream.size();
    std::uint32_t blockId = 0;
    std::size_t offset = 0;

    while (offset < totalLen) {
        std::size_t remaining = totalLen - offset;
        std::size_t thisBlock = (remaining < blockSize) ? remaining : blockSize;

        // 派生此 block 的 RC4 密钥
        std::string blockInput;
        blockInput.reserve(20 + 8);
        blockInput.append(baseKey_);
        // blockId 小端 4 字节 × 2（[MS-OFFCRYPTO] 2.3.6.1）
        std::uint8_t idBytes[4];
        idBytes[0] = static_cast<std::uint8_t>(blockId & 0xFF);
        idBytes[1] = static_cast<std::uint8_t>((blockId >> 8) & 0xFF);
        idBytes[2] = static_cast<std::uint8_t>((blockId >> 16) & 0xFF);
        idBytes[3] = static_cast<std::uint8_t>((blockId >> 24) & 0xFF);
        blockInput.append(reinterpret_cast<const char*>(idBytes), 4);
        blockInput.append(reinterpret_cast<const char*>(idBytes), 4);

        std::string blockHash = internal::sha1(blockInput);
        std::string rc4Key = blockHash.substr(0, keySize);

        // 创建此 block 的 Cipher
        auto cipher = CipherFactory::CreateCipher(CipherAlgorithm::kRC4);
        if (!cipher) return;
        cipher->Init(rc4Key, std::string(), CipherChaining::kUnknown);

        // 解密此 block
        cipher->Update(&stream[offset], 0, static_cast<int>(thisBlock));

        offset += thisBlock;
        ++blockId;
    }
}

void RC4Encryption::DecryptPackageStream(std::string& stream) {
    // 一次性解密（blockSize = stream.size()，使用 blockId=0）
    // 实际场景中应该分块解密
    DecryptBlockedStream(stream, stream.size());
}

std::unique_ptr<Cipher> RC4Encryption::GenerateCipher(std::uint32_t blockId) {
    if (!verified_) return nullptr;

    std::uint32_t keySize = header_.keySize;
    if (keySize == 0 || keySize > 20) keySize = 20;

    // 派生此 block 的 RC4 密钥
    std::string blockInput;
    blockInput.reserve(20 + 8);
    blockInput.append(baseKey_);
    std::uint8_t idBytes[4];
    idBytes[0] = static_cast<std::uint8_t>(blockId & 0xFF);
    idBytes[1] = static_cast<std::uint8_t>((blockId >> 8) & 0xFF);
    idBytes[2] = static_cast<std::uint8_t>((blockId >> 16) & 0xFF);
    idBytes[3] = static_cast<std::uint8_t>((blockId >> 24) & 0xFF);
    blockInput.append(reinterpret_cast<const char*>(idBytes), 4);
    blockInput.append(reinterpret_cast<const char*>(idBytes), 4);

    std::string blockHash = internal::sha1(blockInput);
    std::string rc4Key = blockHash.substr(0, keySize);

    auto cipher = CipherFactory::CreateCipher(CipherAlgorithm::kRC4);
    if (!cipher) return nullptr;
    cipher->Init(rc4Key, std::string(), CipherChaining::kUnknown);
    return cipher;
}

} // namespace crypto
} // namespace office
} // namespace zq
