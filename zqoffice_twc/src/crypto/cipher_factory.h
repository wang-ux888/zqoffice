// =============================================================================
// src/crypto/cipher_factory.h / cipher_factory.cpp
//
// CipherFactory 工厂类（按算法创建 Cipher）
//
//   static unique_ptr<Cipher> CreateCipher(CipherAlgorithm)
//
// 用法：
//   auto cipher = CipherFactory::CreateCipher(CipherAlgorithm::kAES);
//   cipher->Init(key, iv, CipherChaining::kCBC);
//   cipher->Update(data);
// =============================================================================
#pragma once

#include <memory>

#include "cipher.h"
#include "crypto_enums.h"

namespace zq {
namespace office {
namespace crypto {

/// Cipher 工厂
class CipherFactory {
public:
    /// 按算法创建 Cipher
    /// @param algo 算法（AES/RC4）
    /// @return Cipher 实例（不支持算法返回 nullptr）
    static std::unique_ptr<Cipher> CreateCipher(CipherAlgorithm algo);
};

} // namespace crypto
} // namespace office
} // namespace zq
