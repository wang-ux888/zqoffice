// =============================================================================
// src/crypto/crypto_enums.h
//
// 加密相关枚举集
//
//   - CipherAlgorithm：加密算法（AES/RC4/3DES）
//   - CipherChaining：链接模式（CBC/ECB/CFB）
//   - HashAlgorithm：哈希算法（SHA-1/SHA-256/SHA-384/SHA-512/MD5）
//
// 这些枚举被 Cipher/MessageDigest/CipherFactory/MessageDigestFactory 等使用。
// =============================================================================
#pragma once

#include <string>

namespace zq {
namespace office {
namespace crypto {

/// 加密算法
enum class CipherAlgorithm {
    kUnknown = 0,
    kAES     = 1,  // AES（128/192/256）
    kRC4     = 2,  // RC4（Office 97-2003）
    k3DES    = 3,  // Triple DES
};

/// 链接模式
enum class CipherChaining {
    kUnknown = 0,
    kCBC     = 1,  // Cipher Block Chaining
    kECB     = 2,  // Electronic Codebook
    kCFB     = 3,  // Cipher Feedback
};

/// 哈希算法
enum class HashAlgorithm {
    kUnknown  = 0,
    kSHA1     = 1,
    kSHA256   = 2,
    kSHA384   = 3,
    kSHA512   = 4,
    kMD5      = 5,
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

/// CipherAlgorithm → 字符串（"AES"/"RC4"/"3DES"）
inline const char* cipherAlgorithmToString(CipherAlgorithm algo) {
    switch (algo) {
        case CipherAlgorithm::kAES:  return "AES";
        case CipherAlgorithm::kRC4:  return "RC4";
        case CipherAlgorithm::k3DES: return "3DES";
        default: return "unknown";
    }
}

/// 字符串 → CipherAlgorithm（不区分大小写，支持 "AES"/"RC4"/"3DES"/"DES-EDE3-CBC"）
inline CipherAlgorithm parseCipherAlgorithm(const std::string& s) {
    // 转大写比较
    std::string upper;
    upper.reserve(s.size());
    for (char c : s) {
        upper.push_back(static_cast<char>(::toupper(static_cast<unsigned char>(c))));
    }
    if (upper == "AES" || upper.find("AES") != std::string::npos) {
        return CipherAlgorithm::kAES;
    }
    if (upper == "RC4") {
        return CipherAlgorithm::kRC4;
    }
    if (upper == "3DES" || upper.find("3DES") != std::string::npos ||
        upper.find("DES-EDE3") != std::string::npos) {
        return CipherAlgorithm::k3DES;
    }
    return CipherAlgorithm::kUnknown;
}

/// CipherChaining → 字符串（"CBC"/"ECB"/"CFB"）
inline const char* cipherChainingToString(CipherChaining mode) {
    switch (mode) {
        case CipherChaining::kCBC: return "CBC";
        case CipherChaining::kECB: return "ECB";
        case CipherChaining::kCFB: return "CFB";
        default: return "unknown";
    }
}

/// 字符串 → CipherChaining（不区分大小写）
inline CipherChaining parseCipherChaining(const std::string& s) {
    std::string upper;
    upper.reserve(s.size());
    for (char c : s) {
        upper.push_back(static_cast<char>(::toupper(static_cast<unsigned char>(c))));
    }
    if (upper == "CBC" || upper.find("CBC") != std::string::npos) {
        return CipherChaining::kCBC;
    }
    if (upper == "ECB" || upper.find("ECB") != std::string::npos) {
        return CipherChaining::kECB;
    }
    if (upper == "CFB" || upper.find("CFB") != std::string::npos) {
        return CipherChaining::kCFB;
    }
    return CipherChaining::kUnknown;
}

/// HashAlgorithm → 字符串（"SHA-1"/"SHA-256"/"SHA-384"/"SHA-512"/"MD5"）
inline const char* hashAlgorithmToString(HashAlgorithm algo) {
    switch (algo) {
        case HashAlgorithm::kSHA1:   return "SHA-1";
        case HashAlgorithm::kSHA256: return "SHA-256";
        case HashAlgorithm::kSHA384: return "SHA-384";
        case HashAlgorithm::kSHA512: return "SHA-512";
        case HashAlgorithm::kMD5:    return "MD5";
        default: return "unknown";
    }
}

/// 字符串 → HashAlgorithm（不区分大小写，兼容 "SHA1"/"SHA-1"/"SHA_1" 等变体）
inline HashAlgorithm parseHashAlgorithm(const std::string& s) {
    std::string upper;
    upper.reserve(s.size());
    for (char c : s) {
        upper.push_back(static_cast<char>(::toupper(static_cast<unsigned char>(c))));
    }
    // 去掉分隔符 - / _ 统一比较
    auto normalize = [](std::string str) {
        std::string out;
        for (char c : str) {
            if (c == '-' || c == '_' || c == '/') continue;
            out.push_back(c);
        }
        return out;
    };
    std::string n = normalize(upper);
    if (n == "SHA1" || n == "SHA")       return HashAlgorithm::kSHA1;
    if (n == "SHA256")                    return HashAlgorithm::kSHA256;
    if (n == "SHA384")                    return HashAlgorithm::kSHA384;
    if (n == "SHA512")                    return HashAlgorithm::kSHA512;
    if (n == "MD5")                       return HashAlgorithm::kMD5;
    return HashAlgorithm::kUnknown;
}

} // namespace crypto
} // namespace office
} // namespace zq
