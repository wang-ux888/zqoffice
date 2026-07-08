// =============================================================================
// src/crypto/i_encryption.h
//
// IEncryption 接口（加密体系抽象）
//
//   三个具体实现：
//     - XORObfuscation   (Office 97-2003 .xls XOR 混淆)
//     - RC4Encryption    (Office 97-2003 通用 RC4 加密)
//     - EncryptionInfo   (Office 2007+ Agile 加密)
//
//   生命周期：
//     1. EncryptionParser::ParseEncryption03 / ParseEncryptionInfoStream
//        根据 Encryption stream 类型创建具体实现
//     2. VerifyPassword(password) 验证密码并派生密钥
//     3. GenerateCipher(blockId) 取分块解密器
//     4. DecryptBlockedStream(stream, blockSize) 分块解密
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace zq {
namespace office {
namespace crypto {

class Cipher;
class InputStream;  // 前向声明（biff::InputStream 的别名，避免循环依赖）
class NativeEventListener;  // 前向声明（事件监听器，用于 ReportCryptAlgoInfo）

/// 加密接口（抽象基类）
class IEncryption {
public:
    virtual ~IEncryption() = default;

    /// 解析加密流
    /// @param stream 输入流（Encryption stream / EncryptionInfo stream）
    virtual void Parse(InputStream& stream) = 0;

    /// 验证密码（同时派生密钥，验证成功后才能 DecryptBlockedStream）
    /// @param password 用户输入的密码（UTF-8）
    /// @return 密码是否正确
    virtual bool VerifyPassword(const std::string& password) = 0;

    /// 分块解密（对已 VerifyPassword 的加密流进行流式解密）
    /// @param stream 输入流（待解密数据），in-place 解密
    /// @param blockSize 分块大小（字节，Agile 通常 4096）
    virtual void DecryptBlockedStream(std::string& stream, std::size_t blockSize) = 0;

    /// 报告加密算法信息（用于 UI 显示）
    virtual void ReportCryptAlgoInfo(NativeEventListener* listener) = 0;

    /// 生成指定分块的 Cipher
    /// @param blockId 分块 ID（从 0 开始）
    /// @return Cipher 实例（调用方负责释放）
    virtual std::unique_ptr<Cipher> GenerateCipher(std::uint32_t blockId) = 0;
};

} // namespace crypto
} // namespace office
} // namespace zq
