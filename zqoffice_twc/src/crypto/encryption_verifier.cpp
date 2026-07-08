// =============================================================================
// src/crypto/encryption_verifier.cpp
//
// EncryptionVerifier 实现
// =============================================================================
#include "encryption_verifier.h"

namespace zq {
namespace office {
namespace crypto {

// InputStream 是 biff::InputStream 的别名，实际实现由 EncryptionParser 负责
// 这里 Parse 是空实现，具体解析逻辑在 EncryptionParser 中
class InputStream {};

void EncryptionVerifier::Parse(InputStream& /*stream*/) {
    // 实际解析逻辑由 EncryptionParser::ParseEncryption03 实现
    // 因为需要直接访问输入流的字节序和字段布局
}

} // namespace crypto
} // namespace office
} // namespace zq
