// =============================================================================
// src/crypto/cipher.h / cipher.cpp
//
// Cipher 类（加密器抽象）
//
//   抽象基类，派生类：
//     - AesCipher  (AES-128/192/256, CBC/ECB)
//     - RC4Cipher  (RC4 流密码)
//
//   生命周期：
//     1. CipherFactory::CreateCipher(algo) 创建具体实现
//     2. Init(key, iv, chaining) 初始化密钥
//     3. Update(data) 加密/解密（对称密码，in-place 安全）
//
//   注意：Office 加密使用对称密码，加密和解密使用相同流程。
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "crypto_enums.h"

namespace zq {
namespace office {
namespace crypto {

/// 加密器抽象基类
class Cipher {
public:
    virtual ~Cipher() = default;

    /// 初始化加密器
    /// @param key 密钥（原始字节，长度需匹配算法：AES-128=16, AES-192=24, AES-256=32, RC4=任意）
    /// @param iv 初始向量（CBC 模式必需，长度 = 块大小；ECB/RC4 忽略）
    /// @param chaining 链接模式
    virtual void Init(const std::string& key,
                      const std::string& iv,
                      CipherChaining chaining) = 0;

    /// 更新数据（in-place 加密/解密）
    /// @param data 待处理数据（加密/解密后写回 data）
    virtual void Update(std::string& data) = 0;

    /// 更新数据（指针版本）
    /// @param data 数据指针（in-place）
    /// @param offset 偏移量
    /// @param length 长度
    virtual void Update(void* data, int offset, int length) = 0;

    /// 取算法
    virtual CipherAlgorithm GetAlgorithm() const = 0;

    /// 取块大小（AES=16, RC4=1）
    virtual std::size_t GetBlockSize() const = 0;
};

// ---------------------------------------------------------------------------
// 派生类工厂函数（实现细节隐藏在 cipher.cpp，避免暴露具体类定义）
// ---------------------------------------------------------------------------

/// 创建 AES Cipher（AES-128/192/256，密钥长度决定位数）
std::unique_ptr<Cipher> CreateAesCipher();

/// 创建 RC4 Cipher
std::unique_ptr<Cipher> CreateRC4Cipher();

} // namespace crypto
} // namespace office
} // namespace zq
