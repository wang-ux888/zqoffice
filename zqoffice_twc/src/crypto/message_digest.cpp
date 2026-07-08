// =============================================================================
// src/crypto/message_digest.cpp
//
// MessageDigest 派生类实现（SHA1Digest / SHA256Digest / SHA384Digest / SHA512Digest）
// =============================================================================
#include "message_digest.h"

#include <memory>
#include <stdexcept>

#include "internal/sha.h"

namespace zq {
namespace office {
namespace crypto {

// ---------------------------------------------------------------------------
// 基础实现：使用 internal::sha* 函数
// 由于 Office 加密场景下摘要计算大多是"一次性输入"，这里采用简化策略：
//   - 不维护增量状态（每次 Update 累积到 buffer，Digest 时一次性计算）
//   - Digest() 后清空 buffer
// 这样实现简单且对 Office 场景足够（Agile 加密的 HMK/SK 派生都是一次性输入）
// ---------------------------------------------------------------------------

/// 通用 MessageDigest 实现（基于一次性 hash 函数）
template <HashAlgorithm Algo, std::size_t DigestSize>
class GenericDigest : public MessageDigest {
public:
    void Update(const std::string& data) override {
        buffer_.append(data);
    }

    void Update(const void* data, int offset, int length) override {
        if (length <= 0) return;
        const auto* p = static_cast<const char*>(data) + offset;
        buffer_.append(p, static_cast<std::size_t>(length));
    }

    std::string Digest() override {
        std::string result;
        result.resize(DigestSize);
        computeHash(buffer_.data(), buffer_.size(),
                    reinterpret_cast<std::uint8_t*>(&result[0]));
        buffer_.clear();
        return result;
    }

    std::string Digest(const std::string& data) override {
        std::string result;
        result.resize(DigestSize);
        computeHash(data.data(), data.size(),
                    reinterpret_cast<std::uint8_t*>(&result[0]));
        return result;
    }

    std::string Digest(const void* data, int offset, int length) override {
        std::string result;
        result.resize(DigestSize);
        const auto* p = static_cast<const char*>(data) + offset;
        computeHash(p, static_cast<std::size_t>(length),
                    reinterpret_cast<std::uint8_t*>(&result[0]));
        return result;
    }

    HashAlgorithm GetAlgorithm() const override { return Algo; }

    std::size_t GetDigestSize() const override { return DigestSize; }

private:
    /// 调用对应的 hash 函数
    static void computeHash(const void* data, std::size_t len, std::uint8_t* out);

    std::string buffer_;  // 累积的输入数据
};

// 模板特化：computeHash
template <>
void GenericDigest<HashAlgorithm::kSHA1, 20>::computeHash(
    const void* data, std::size_t len, std::uint8_t* out) {
    internal::sha1(data, len, out);
}

template <>
void GenericDigest<HashAlgorithm::kSHA256, 32>::computeHash(
    const void* data, std::size_t len, std::uint8_t* out) {
    internal::sha256(data, len, out);
}

template <>
void GenericDigest<HashAlgorithm::kSHA384, 48>::computeHash(
    const void* data, std::size_t len, std::uint8_t* out) {
    internal::sha384(data, len, out);
}

template <>
void GenericDigest<HashAlgorithm::kSHA512, 64>::computeHash(
    const void* data, std::size_t len, std::uint8_t* out) {
    internal::sha512(data, len, out);
}

// ---------------------------------------------------------------------------
// 工厂函数
// ---------------------------------------------------------------------------
std::unique_ptr<MessageDigest> CreateSHA1Digest() {
    return std::make_unique<GenericDigest<HashAlgorithm::kSHA1, 20>>();
}

std::unique_ptr<MessageDigest> CreateSHA256Digest() {
    return std::make_unique<GenericDigest<HashAlgorithm::kSHA256, 32>>();
}

std::unique_ptr<MessageDigest> CreateSHA384Digest() {
    return std::make_unique<GenericDigest<HashAlgorithm::kSHA384, 48>>();
}

std::unique_ptr<MessageDigest> CreateSHA512Digest() {
    return std::make_unique<GenericDigest<HashAlgorithm::kSHA512, 64>>();
}

} // namespace crypto
} // namespace office
} // namespace zq
