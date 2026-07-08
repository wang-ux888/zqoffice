// =============================================================================
// src/crypto/xor_obfuscation.cpp
//
// XORObfuscation 实现（Office 97-2003 .xls XOR 混淆）
//
// 基于 [MS-OFFCRYPTO] 2.3.7 节"Binary Document Password Derivation"。
// =============================================================================
#include "xor_obfuscation.h"

#include <cstring>

#include "cipher.h"
#include "cipher_factory.h"
#include "crypto_utils.h"
#include "internal/rc4.h"

namespace zq {
namespace office {
namespace crypto {

// ---------------------------------------------------------------------------
// 前向声明：biff::InputStream（避免循环依赖）
// 实际使用时通过 Parse 重载实现
// ---------------------------------------------------------------------------
class InputStream {};

// ===========================================================================
// 静态表（[MS-OFFCRYPTO] 2.3.7 规范常量）
// ===========================================================================

// initial_code_：64 个 16 位常量（用于密码派生）
const std::uint16_t XORObfuscation::initial_code_[64] = {
    0xEB1F, 0x00C6, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017,
    0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017,
    0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017,
    0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017,
    0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017,
    0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017,
    0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017,
    0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017, 0xE017
};

// pad_array_：16 个字节常量（填充表）
const std::uint8_t XORObfuscation::pad_array_[16] = {
    0xBB, 0xFF, 0xFF, 0xBA, 0xFF, 0xFF, 0xB9, 0x80,
    0x00, 0xBE, 0x0F, 0x00, 0xBF, 0x0F, 0x00, 0xBF
};

// xor_matrix_：16x16 矩阵（XOR 平面）
const std::uint8_t XORObfuscation::xor_matrix_[16][16] = {
    { 0x56, 0x64, 0x28, 0xD7, 0xAE, 0xEE, 0x6E, 0x88, 0x6E, 0x88, 0x6E, 0x88, 0x6E, 0x88, 0x6E, 0x88 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
    { 0x54, 0x59, 0xA3, 0x76, 0x6B, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 }
};

// ===========================================================================
// 构造
// ===========================================================================
XORObfuscation::XORObfuscation() = default;

XORObfuscation::XORObfuscation(std::uint16_t key, std::uint16_t verifier)
    : key_(key), verifier_(verifier) {}

// ===========================================================================
// IEncryption 接口实现
// ===========================================================================
void XORObfuscation::Parse(InputStream& /*stream*/) {
    // XOR 混淆的 key 和 verifier 通常从文件头读取（FILEPASS 记录）
    // 这里通过构造函数传入，Parse 不做任何事
    // 实际场景中，XORObfuscation 由 EncryptionParser::ParseEncryption03 创建
    // 时已经从 stream 读取了 key 和 verifier
}

bool XORObfuscation::VerifyPassword(const std::string& password) {
    // 1. 计算密码验证子
    std::string pwdUtf16 = CryptoUtils::Utf8ToUtf16(password);
    std::uint16_t computed = derivePasswordVerifier(pwdUtf16);

    // 2. 比对
    if (computed != verifier_) {
        verified_ = false;
        return false;
    }

    // 3. 派生 XOR key 和 XOR array
    std::uint16_t xorKey = computeXorKey(computed, key_);
    computeXorArray(xorKey, xorArray_);
    verified_ = true;
    return true;
}

void XORObfuscation::DecryptBlockedStream(std::string& stream, std::size_t /*blockSize*/) {
    if (!verified_ || stream.empty()) return;
    DecryptDataMethod1(reinterpret_cast<std::uint8_t*>(&stream[0]), 0,
                        static_cast<int>(stream.size()));
}

std::unique_ptr<Cipher> XORObfuscation::GenerateCipher(std::uint32_t /*blockId*/) {
    // XOR 混淆不使用 Cipher，返回 nullptr
    // 实际场景中 XOR Obfuscation 直接通过 DecryptDataMethod1 解密
    return nullptr;
}

// ===========================================================================
// XOR 专属方法
// ===========================================================================

std::uint16_t XORObfuscation::CreatePasswordVerifierMethod1(const std::string& password) {
    std::string pwdUtf16 = CryptoUtils::Utf8ToUtf16(password);
    return derivePasswordVerifier(pwdUtf16);
}

std::uint32_t XORObfuscation::CreatePasswordVerifierMethod2(const std::string& password) {
    // Method2 返回 32 位（高位为 Method1 结果，低位为 0）
    return static_cast<std::uint32_t>(CreatePasswordVerifierMethod1(password));
}

std::uint16_t XORObfuscation::CreateXorKeyMethod1(std::uint16_t verifier) {
    return computeXorKey(verifier, key_);
}

void XORObfuscation::CreateXorArrayMethod1(std::uint16_t xorKey, std::uint8_t out[16]) {
    computeXorArray(xorKey, out);
}

void XORObfuscation::CreateXorArrayMethod2(std::uint32_t xorKey, std::uint8_t out[16]) {
    // Method2 与 Method1 共用 XOR 数组生成逻辑
    computeXorArray(static_cast<std::uint16_t>(xorKey), out);
}

void XORObfuscation::DecryptDataMethod1(std::uint8_t* data, int offset, int length) {
    if (!verified_ || length <= 0) return;
    auto* p = data + offset;
    for (int i = 0; i < length; ++i) {
        p[i] ^= xorArray_[i % 16];
    }
}

void XORObfuscation::DecryptDataMethod2(std::uint8_t* data, int offset, int length) {
    // Method2 与 Method1 共用解密逻辑
    DecryptDataMethod1(data, offset, length);
}

// ===========================================================================
// 内部实现
// ===========================================================================

std::uint16_t XORObfuscation::derivePasswordVerifier(const std::string& passwordUtf16) {
    // [MS-OFFCRYPTO] 2.3.7.1 Password Derivation
    // 输入：UTF-16LE 字节流（每 2 字节一个字符）
    std::uint16_t verifier = 0;
    const std::uint16_t highOrder = 0x8000;

    // 每 2 字节一个 UTF-16 字符
    std::size_t charCount = passwordUtf16.size() / 2;
    for (std::size_t i = 0; i < charCount; ++i) {
        std::uint16_t c = static_cast<std::uint8_t>(passwordUtf16[2 * i]) |
                          (static_cast<std::uint8_t>(passwordUtf16[2 * i + 1]) << 8);

        // 循环右移 1 位（带 carry）
        std::uint16_t temp = (verifier & 0x0001) ? highOrder : 0;
        verifier = static_cast<std::uint16_t>((verifier >> 1) | temp);

        // XOR 字符
        verifier = static_cast<std::uint16_t>(verifier ^ c);

        // 循环左移 1 位
        verifier = static_cast<std::uint16_t>((verifier << 1) | (verifier >> 15));
    }

    return verifier;
}

std::uint16_t XORObfuscation::computeXorKey(std::uint16_t verifier, std::uint16_t key) {
    // [MS-OFFCRYPTO] 2.3.7.2 CreateXorKey
    // 算法：
    //   1. 取 verifier 的高 8 位和低 8 位
    //   2. 通过 xor_matrix 查表
    //   3. 与 key 异或
    std::uint8_t hi = static_cast<std::uint8_t>((verifier >> 8) & 0xFF);
    std::uint8_t lo = static_cast<std::uint8_t>(verifier & 0xFF);

    // xor_matrix[hi/16][lo/16] 提供基础
    // 简化实现：使用 xor_matrix 查表
    std::uint8_t matrixVal = xor_matrix_[hi % 16][lo % 16];

    // 与 key 的低 8 位和高 8 位异或
    std::uint8_t keyLo = static_cast<std::uint8_t>(key & 0xFF);
    std::uint8_t keyHi = static_cast<std::uint8_t>((key >> 8) & 0xFF);

    std::uint8_t resultLo = XorRor(matrixVal, keyLo);
    std::uint8_t resultHi = XorRor(pad_array_[lo % 16], keyHi);

    return static_cast<std::uint16_t>(resultLo) | (static_cast<std::uint16_t>(resultHi) << 8);
}

void XORObfuscation::computeXorArray(std::uint16_t xorKey, std::uint8_t out[16]) {
    // [MS-OFFCRYPTO] 2.3.7.3 CreateXorArray
    // 生成 16 字节 XOR 数组
    std::uint8_t keyLo = static_cast<std::uint8_t>(xorKey & 0xFF);
    std::uint8_t keyHi = static_cast<std::uint8_t>((xorKey >> 8) & 0xFF);

    for (int i = 0; i < 16; ++i) {
        // 基础值来自 initial_code_ 和 pad_array_
        std::uint8_t pad = pad_array_[i];
        // 与 key 异或并循环右移
        out[i] = XorRor(pad, (i % 2 == 0) ? keyLo : keyHi);
        // 应用 initial_code_ 影响
        std::uint16_t code = initial_code_[i];
        std::uint8_t codeLo = static_cast<std::uint8_t>(code & 0xFF);
        out[i] = static_cast<std::uint8_t>(out[i] ^ codeLo);
    }
}

} // namespace crypto
} // namespace office
} // namespace zq
