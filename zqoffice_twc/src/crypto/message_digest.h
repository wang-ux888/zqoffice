// =============================================================================
// src/crypto/message_digest.h / message_digest.cpp
//
// MessageDigest 类（哈希摘要抽象）
//
//   抽象基类，派生类：
//     - SHA1Digest   (SHA-1,  20 字节摘要)
//     - SHA256Digest (SHA-256,32 字节摘要)
//     - SHA384Digest (SHA-384,48 字节摘要)
//     - SHA512Digest (SHA-512,64 字节摘要)
//     - MD5Digest    (MD5,   16 字节摘要)
//
//   生命周期：
//     1. MessageDigestFactory::CreateMessageDigest(algo) 创建具体实现
//     2. Update(data) 增量更新（可多次调用）
//     3. Digest() 取最终摘要（finalize 后不能再 Update）
//     或：Digest(data) 一次性计算摘要
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "crypto_enums.h"

namespace zq {
namespace office {
namespace crypto {

/// 哈希摘要抽象基类
class MessageDigest {
public:
    virtual ~MessageDigest() = default;

    /// 增量更新（可多次调用，最终通过 Digest() 取结果）
    /// @param data 数据
    virtual void Update(const std::string& data) = 0;

    /// 增量更新（指针版本）
    /// @param data 数据指针
    /// @param offset 偏移量
    /// @param length 长度
    virtual void Update(const void* data, int offset, int length) = 0;

    /// 取最终摘要（finalize，调用后状态重置，可继续 Update）
    /// @return 摘要（原始字节，长度 = GetDigestSize()）
    virtual std::string Digest() = 0;

    /// 一次性计算摘要（便捷方法，等于 Update + Digest）
    /// @param data 数据
    /// @return 摘要
    virtual std::string Digest(const std::string& data) = 0;

    /// 一次性计算摘要（指针版本）
    /// @param data 数据指针
    /// @param offset 偏移量
    /// @param length 长度
    /// @return 摘要
    virtual std::string Digest(const void* data, int offset, int length) = 0;

    /// 取算法
    virtual HashAlgorithm GetAlgorithm() const = 0;

    /// 取摘要长度（字节）
    virtual std::size_t GetDigestSize() const = 0;
};

// ---------------------------------------------------------------------------
// 派生类工厂函数（实现细节隐藏在 message_digest.cpp）
// ---------------------------------------------------------------------------

std::unique_ptr<MessageDigest> CreateSHA1Digest();
std::unique_ptr<MessageDigest> CreateSHA256Digest();
std::unique_ptr<MessageDigest> CreateSHA384Digest();
std::unique_ptr<MessageDigest> CreateSHA512Digest();

} // namespace crypto
} // namespace office
} // namespace zq
