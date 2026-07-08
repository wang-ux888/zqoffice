// =============================================================================
// src/crypto/message_digest_factory.h / message_digest_factory.cpp
//
// MessageDigestFactory 工厂类（按算法创建 MessageDigest）
//
//   static unique_ptr<MessageDigest> CreateMessageDigest(HashAlgorithm)
// =============================================================================
#pragma once

#include <memory>

#include "crypto_enums.h"
#include "message_digest.h"

namespace zq {
namespace office {
namespace crypto {

/// MessageDigest 工厂
class MessageDigestFactory {
public:
    /// 按算法创建 MessageDigest
    /// @param algo 哈希算法（SHA-1/SHA-256/SHA-384/SHA-512）
    /// @return MessageDigest 实例（不支持算法返回 nullptr）
    static std::unique_ptr<MessageDigest> CreateMessageDigest(HashAlgorithm algo);
};

} // namespace crypto
} // namespace office
} // namespace zq
