// =============================================================================
// src/crypto/cipher_factory.cpp
//
// CipherFactory 实现（按算法创建 Cipher）
// =============================================================================
#include "cipher_factory.h"

#include "cipher.h"

namespace zq {
namespace office {
namespace crypto {

std::unique_ptr<Cipher> CipherFactory::CreateCipher(CipherAlgorithm algo) {
    switch (algo) {
        case CipherAlgorithm::kAES:
            return CreateAesCipher();
        case CipherAlgorithm::kRC4:
            return CreateRC4Cipher();
        // 3DES 暂未实现（Office 实际场景几乎不用）
        case CipherAlgorithm::k3DES:
        case CipherAlgorithm::kUnknown:
        default:
            return nullptr;
    }
}

} // namespace crypto
} // namespace office
} // namespace zq
