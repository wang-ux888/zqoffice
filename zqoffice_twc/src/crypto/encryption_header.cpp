// =============================================================================
// src/crypto/encryption_header.cpp
//
// EncryptionHeader 实现
// =============================================================================
#include "encryption_header.h"

#include "cipher_factory.h"
#include "message_digest_factory.h"

namespace zq {
namespace office {
namespace crypto {

std::unique_ptr<Cipher> EncryptionHeader::CreateCipher() const {
    return CipherFactory::CreateCipher(GetCipherAlgorithm());
}

std::unique_ptr<MessageDigest> EncryptionHeader::CreateMessageDigest() const {
    return MessageDigestFactory::CreateMessageDigest(GetHashAlgorithm());
}

} // namespace crypto
} // namespace office
} // namespace zq
