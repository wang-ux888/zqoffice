// =============================================================================
// src/crypto/xor_obfuscation.h / xor_obfuscation.cpp
//
// XORObfuscation 类（Office 97-2003 .xls XOR 混淆）
//
//   基于 [MS-OFFCRYPTO] 2.3.7 节"Binary Document Password Derivation"实现。
//
//   三个静态表（initial_code_ / pad_array_ / xor_matrix_）源自规范。
//
//   算法流程：
//     1. VerifyPassword(password):
//        a. CreatePasswordVerifierMethod1(password) → 16-bit verifier
//        b. 与文件中存储的 passwordVerifier 比对
//        c. CreateXorKeyMethod1() → 8-bit key
//        d. CreateXorArrayMethod1() → 16-byte xor array
//     2. DecryptDataMethod1(data): data[i] ^= xor_array[i % 16]
//
//   注意：XOR 混淆不是真正的加密，仅用于兼容 Office 97-2003 .xls 文件。
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "crypto_enums.h"
#include "i_encryption.h"

namespace zq {
namespace office {
namespace crypto {

class InputStream;

/// XOR 混淆（Office 97-2003 .xls）
class XORObfuscation : public IEncryption {
public:
    /// 默认构造
    XORObfuscation();

    /// 带参数构造（key + verifier，从文件头解析后传入）
    /// @param key 16 位 XOR key
    /// @param verifier 16 位 password verifier
    XORObfuscation(std::uint16_t key, std::uint16_t verifier);

    ~XORObfuscation() override = default;

    // ---------------------------------------------------------------------------
    // IEncryption 接口实现
    // ---------------------------------------------------------------------------
    void Parse(InputStream& stream) override;
    bool VerifyPassword(const std::string& password) override;
    void DecryptBlockedStream(std::string& stream, std::size_t blockSize) override;
    void ReportCryptAlgoInfo(NativeEventListener* /*listener*/) override {
        // 简化实现：不做任何事（NativeEventListener 是事件监听器，用于 UI 显示）
    }
    std::unique_ptr<Cipher> GenerateCipher(std::uint32_t blockId) override;

    // ---------------------------------------------------------------------------
    // ---------------------------------------------------------------------------

    /// 创建密码验证子（Method1）
    /// @param password 密码（UTF-8，内部转 UTF-16LE）
    /// @return 16 位验证值
    std::uint16_t CreatePasswordVerifierMethod1(const std::string& password);

    /// 创建密码验证子（Method2，通用版本，返回 32 位）
    /// @param password 密码（UTF-8，内部转 UTF-16LE）
    /// @return 32 位验证值
    std::uint32_t CreatePasswordVerifierMethod2(const std::string& password);

    /// 创建 XOR 密钥（Method1）
    /// @param verifier 密码验证子
    /// @return 16 位 XOR key
    std::uint16_t CreateXorKeyMethod1(std::uint16_t verifier);

    /// 创建 XOR 数组（Method1）
    /// @param xorKey XOR 密钥
    /// @return 16 字节 XOR 数组
    void CreateXorArrayMethod1(std::uint16_t xorKey, std::uint8_t out[16]);

    /// 创建 XOR 数组（Method2，通用版本）
    void CreateXorArrayMethod2(std::uint32_t xorKey, std::uint8_t out[16]);

    /// 解密数据（Method1）
    /// @param data 数据（in-place）
    /// @param offset 偏移量
    /// @param length 长度
    void DecryptDataMethod1(std::uint8_t* data, int offset, int length);

    /// 解密数据（Method2）
    void DecryptDataMethod2(std::uint8_t* data, int offset, int length);

    /// 8 位循环右移
    static std::uint8_t Ror(std::uint8_t v, int n) {
        n &= 7;
        return static_cast<std::uint8_t>((v >> n) | (v << (8 - n)));
    }

    /// XOR 后循环右移（XorRor）
    /// @param a 操作数 a
    /// @param b 操作数 b
    /// @return (a XOR b) 循环右移 1 位
    static std::uint8_t XorRor(std::uint8_t a, std::uint8_t b) {
        return Ror(static_cast<std::uint8_t>(a ^ b), 1);
    }

    // ---------------------------------------------------------------------------
    // 静态表（[MS-OFFCRYPTO] 2.3.7 规范常量）
    // ---------------------------------------------------------------------------
    static const std::uint16_t initial_code_[64];
    static const std::uint8_t  pad_array_[16];
    static const std::uint8_t  xor_matrix_[16][16];

private:
    /// 执行 XOR 混淆密码派生（[MS-OFFCRYPTO] 2.3.7.1）
    /// @param password 密码（UTF-16LE 字节流）
    /// @return 16 位 password verifier
    std::uint16_t derivePasswordVerifier(const std::string& passwordUtf16);

    /// 计算 XOR key（基于 verifier 和文件中存储的 key）
    std::uint16_t computeXorKey(std::uint16_t verifier, std::uint16_t key);

    /// 计算 XOR array（基于 xorKey）
    void computeXorArray(std::uint16_t xorKey, std::uint8_t out[16]);

    // 状态
    std::uint16_t key_ = 0;          // 文件头中存储的 16 位 key
    std::uint16_t verifier_ = 0;     // 文件头中存储的 16 位 verifier
    bool verified_ = false;          // VerifyPassword 是否通过
    std::uint8_t  xorArray_[16] = {0};  // 验证密码后派生的 XOR 数组
};

} // namespace crypto
} // namespace office
} // namespace zq
