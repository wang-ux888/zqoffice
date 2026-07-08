// =============================================================================
// src/crypto/cipher.cpp
//
// Cipher 派生类实现（AesCipher / RC4Cipher）
// =============================================================================
#include "cipher.h"

#include <cstring>
#include <stdexcept>

#include "internal/aes.h"
#include "internal/rc4.h"

namespace zq {
namespace office {
namespace crypto {

// ===========================================================================
// AesCipher（AES-128/192/256 CBC/ECB）
// ===========================================================================
class AesCipher : public Cipher {
public:
    AesCipher() = default;
    ~AesCipher() override = default;

    void Init(const std::string& key,
              const std::string& iv,
              CipherChaining chaining) override {
        // 根据 key 长度选择 AES-128/192/256
        int bits;
        switch (key.size()) {
            case 16: bits = 128; break;
            case 24: bits = 192; break;
            case 32: bits = 256; break;
            default:
                throw std::invalid_argument("AesCipher: invalid key size (must be 16/24/32)");
        }
        if (internal::aesSetDecryptKey(key.data(), bits, decKey_) != 0) {
            throw std::runtime_error("AesCipher: aesSetDecryptKey failed");
        }
        if (internal::aesSetEncryptKey(key.data(), bits, encKey_) != 0) {
            throw std::runtime_error("AesCipher: aesSetEncryptKey failed");
        }
        chaining_ = chaining;
        // IV 必须是 16 字节（CBC 模式）
        if (chaining == CipherChaining::kCBC) {
            if (iv.size() != 16) {
                throw std::invalid_argument("AesCipher: CBC mode requires 16-byte IV");
            }
            std::memcpy(iv_, iv.data(), 16);
            ivInitialized_ = true;
        }
    }

    void Update(std::string& data) override {
        if (data.empty()) return;
        Update(&data[0], 0, static_cast<int>(data.size()));
    }

    void Update(void* data, int offset, int length) override {
        if (length <= 0) return;
        auto* p = static_cast<std::uint8_t*>(data) + offset;
        // Office 加密使用对称密码，加密和解密使用相同流程
        // 但 CBC 模式下加密和解密的 IV 处理不同，这里默认走解密流程（Office 场景以解密为主）
        if (chaining_ == CipherChaining::kCBC) {
            if (!ivInitialized_) {
                throw std::runtime_error("AesCipher: CBC mode requires IV before Update");
            }
            // 解密
            std::uint8_t ivCopy[16];
            std::memcpy(ivCopy, iv_, 16);
            internal::aesCbcDecrypt(decKey_, ivCopy, p, p, static_cast<std::size_t>(length));
        } else if (chaining_ == CipherChaining::kECB) {
            // ECB 模式：逐块解密
            for (int i = 0; i < length; i += 16) {
                internal::aesDecryptBlock(decKey_, p + i, p + i);
            }
        } else {
            throw std::runtime_error("AesCipher: unsupported chaining mode");
        }
    }

    CipherAlgorithm GetAlgorithm() const override { return CipherAlgorithm::kAES; }

    std::size_t GetBlockSize() const override { return internal::kAesBlockSize; }

private:
    internal::AESKey encKey_;
    internal::AESKey decKey_;
    CipherChaining chaining_ = CipherChaining::kUnknown;
    std::uint8_t iv_[16] = {0};
    bool ivInitialized_ = false;
};

// ===========================================================================
// RC4Cipher（RC4 流密码）
// ===========================================================================
class RC4Cipher : public Cipher {
public:
    RC4Cipher() = default;
    ~RC4Cipher() override = default;

    void Init(const std::string& key,
              const std::string& /*iv*/,
              CipherChaining /*chaining*/) override {
        if (key.empty()) {
            throw std::invalid_argument("RC4Cipher: empty key");
        }
        internal::rc4Init(state_, key.data(), key.size());
    }

    void Update(std::string& data) override {
        if (data.empty()) return;
        Update(&data[0], 0, static_cast<int>(data.size()));
    }

    void Update(void* data, int offset, int length) override {
        if (length <= 0) return;
        auto* p = static_cast<std::uint8_t*>(data) + offset;
        internal::rc4Process(state_, p, static_cast<std::size_t>(length));
    }

    CipherAlgorithm GetAlgorithm() const override { return CipherAlgorithm::kRC4; }

    std::size_t GetBlockSize() const override { return 1; }

private:
    internal::RC4State state_;
};

// ---------------------------------------------------------------------------
// 工厂函数
// ---------------------------------------------------------------------------
std::unique_ptr<Cipher> CreateAesCipher() {
    return std::make_unique<AesCipher>();
}

std::unique_ptr<Cipher> CreateRC4Cipher() {
    return std::make_unique<RC4Cipher>();
}

} // namespace crypto
} // namespace office
} // namespace zq
