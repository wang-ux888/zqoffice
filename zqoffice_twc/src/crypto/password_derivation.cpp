// =============================================================================
// src/crypto/password_derivation.cpp
//
// PasswordDerivation 实现
// =============================================================================
#include "password_derivation.h"

#include <stdexcept>

#include "internal/sha.h"

namespace zq {
namespace office {
namespace crypto {

// ---------------------------------------------------------------------------
// XOR 混淆密码验证子（[MS-OFFCRYPTO] 2.3.7.1）
// ---------------------------------------------------------------------------
std::uint16_t PasswordDerivation::DeriveXorVerifier(
    const std::string& passwordUtf16) {
    std::uint16_t verifier = 0;
    const std::uint16_t highOrder = 0x8000;

    std::size_t charCount = passwordUtf16.size() / 2;
    for (std::size_t i = 0; i < charCount; ++i) {
        std::uint16_t c = static_cast<std::uint8_t>(passwordUtf16[2 * i]) |
                          (static_cast<std::uint8_t>(passwordUtf16[2 * i + 1]) << 8);

        std::uint16_t temp = (verifier & 0x0001) ? highOrder : 0;
        verifier = static_cast<std::uint16_t>((verifier >> 1) | temp);
        verifier = static_cast<std::uint16_t>(verifier ^ c);
        verifier = static_cast<std::uint16_t>((verifier << 1) | (verifier >> 15));
    }

    return verifier;
}

// ---------------------------------------------------------------------------
// RC4 加密密码派生（[MS-OFFCRYPTO] 2.3.6.1）
// ---------------------------------------------------------------------------
std::string PasswordDerivation::DeriveRC4Key(
    const std::string& passwordUtf16,
    const std::string& salt,
    std::uint32_t keySize) {
    // H0 = SHA-1(password_utf16 + salt)
    std::string input = passwordUtf16 + salt;
    std::string h0 = internal::sha1(input);

    // RC4 key = H0[0:keySize]
    if (keySize > h0.size()) keySize = static_cast<std::uint32_t>(h0.size());
    return h0.substr(0, keySize);
}

std::string PasswordDerivation::DeriveRC4BlockKey(
    const std::string& baseKey,
    std::uint32_t blockId,
    std::uint32_t keySize) {
    // H_block = SHA-1(H0 + blockId_LE + blockId_LE)
    std::string blockInput;
    blockInput.reserve(20 + 8);
    blockInput.append(baseKey);

    std::uint8_t idBytes[4];
    idBytes[0] = static_cast<std::uint8_t>(blockId & 0xFF);
    idBytes[1] = static_cast<std::uint8_t>((blockId >> 8) & 0xFF);
    idBytes[2] = static_cast<std::uint8_t>((blockId >> 16) & 0xFF);
    idBytes[3] = static_cast<std::uint8_t>((blockId >> 24) & 0xFF);
    blockInput.append(reinterpret_cast<const char*>(idBytes), 4);
    blockInput.append(reinterpret_cast<const char*>(idBytes), 4);

    std::string blockHash = internal::sha1(blockInput);
    if (keySize > blockHash.size()) keySize = static_cast<std::uint32_t>(blockHash.size());
    return blockHash.substr(0, keySize);
}

// ---------------------------------------------------------------------------
// Agile 加密 HMAC 迭代派生（[MS-OFFCRYPTO] 2.3.4.9）
// ---------------------------------------------------------------------------
std::string PasswordDerivation::DeriveAgileHash(
    const std::string& password,
    const std::string& salt,
    std::uint32_t spinCount,
    HashAlgorithm hashAlg) {
    // 第 1 次：HMAC(hashAlg, password, salt)
    std::string h;
    switch (hashAlg) {
        case HashAlgorithm::kSHA1:   h = internal::hmacSha1(password, salt); break;
        case HashAlgorithm::kSHA256: h = internal::hmacSha256(password, salt); break;
        case HashAlgorithm::kSHA384: h = internal::hmacSha384(password, salt); break;
        case HashAlgorithm::kSHA512: h = internal::hmacSha512(password, salt); break;
        default: throw std::runtime_error("Unsupported hash algorithm");
    }

    // 迭代 spinCount - 1 次：h = HMAC(hashAlg, h_prev, "")
    std::string empty;
    for (std::uint32_t i = 1; i < spinCount; ++i) {
        switch (hashAlg) {
            case HashAlgorithm::kSHA1:   h = internal::hmacSha1(h, empty); break;
            case HashAlgorithm::kSHA256: h = internal::hmacSha256(h, empty); break;
            case HashAlgorithm::kSHA384: h = internal::hmacSha384(h, empty); break;
            case HashAlgorithm::kSHA512: h = internal::hmacSha512(h, empty); break;
            default: throw std::runtime_error("Unsupported hash algorithm");
        }
    }

    return h;
}

// ---------------------------------------------------------------------------
// Agile 加密 blockKey 派生（[MS-OFFCRYPTO] 2.3.4.10）
// ---------------------------------------------------------------------------
std::string PasswordDerivation::DeriveBlockKey(
    const std::string& hashValue,
    const std::string& blockKey,
    HashAlgorithm hashAlg) {
    // HMAC(hashAlg, hashValue, blockKey)
    switch (hashAlg) {
        case HashAlgorithm::kSHA1:   return internal::hmacSha1(hashValue, blockKey);
        case HashAlgorithm::kSHA256: return internal::hmacSha256(hashValue, blockKey);
        case HashAlgorithm::kSHA384: return internal::hmacSha384(hashValue, blockKey);
        case HashAlgorithm::kSHA512: return internal::hmacSha512(hashValue, blockKey);
        default: throw std::runtime_error("Unsupported hash algorithm");
    }
}

// ---------------------------------------------------------------------------
// Agile 加密分段密钥派生（[MS-OFFCRYPTO] 2.3.4.11）
// ---------------------------------------------------------------------------
std::string PasswordDerivation::DeriveSegmentKey(
    const std::string& hashValue,
    const std::string& blockKey,
    HashAlgorithm hashAlg,
    std::uint32_t keySize) {
    std::string full = DeriveBlockKey(hashValue, blockKey, hashAlg);
    if (keySize > full.size()) keySize = static_cast<std::uint32_t>(full.size());
    return full.substr(0, keySize);
}

} // namespace crypto
} // namespace office
} // namespace zq
