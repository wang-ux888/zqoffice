// =============================================================================
// src/crypto/message_digest_factory.cpp
//
// MessageDigestFactory 实现（按算法创建 MessageDigest）
// =============================================================================
#include "message_digest_factory.h"

#include "message_digest.h"

namespace zq {
namespace office {
namespace crypto {

std::unique_ptr<MessageDigest> MessageDigestFactory::CreateMessageDigest(
    HashAlgorithm algo) {
    switch (algo) {
        case HashAlgorithm::kSHA1:   return CreateSHA1Digest();
        case HashAlgorithm::kSHA256: return CreateSHA256Digest();
        case HashAlgorithm::kSHA384: return CreateSHA384Digest();
        case HashAlgorithm::kSHA512: return CreateSHA512Digest();
        // MD5 暂未实现（Office 实际场景不用）
        case HashAlgorithm::kMD5:
        case HashAlgorithm::kUnknown:
        default:
            return nullptr;
    }
}

} // namespace crypto
} // namespace office
} // namespace zq
