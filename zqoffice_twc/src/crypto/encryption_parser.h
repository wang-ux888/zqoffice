// =============================================================================
// src/crypto/encryption_parser.h / encryption_parser.cpp
//
// EncryptionParser 类（加密流分发器）
//
//   static unique_ptr<IEncryption> ParseEncryption03(InputStream&)
//     - 解析 Office 97-2003 Encryption stream（二进制格式）
//     - 根据 EncryptionVersionInfo 判断：XOR / RC4
//
//   static unique_ptr<IEncryption> ParseEncryptionInfoStream(const std::string& xml)
//     - 解析 Office 2007+ EncryptionInfo stream（XML 格式）
//     - 使用 AgileEncryptionHandler SAX 解析 XML
//     - 返回 EncryptionInfo 实例（内部持有 AgileEncryption 节点）
// =============================================================================
#pragma once

#include <memory>
#include <string>

#include "i_encryption.h"

namespace zq {
namespace office {
namespace crypto {

class InputStream;

/// 加密流分发器
class EncryptionParser {
public:
    /// 解析 Office 97-2003 加密流
    /// @param stream 输入流（OLE2 Encryption stream 内容）
    /// @return IEncryption 实现（XORObfuscation 或 RC4Encryption）
    /// @note 该方法直接接受字节流（避免 InputStream 接口耦合）
    static std::unique_ptr<IEncryption> ParseEncryption03(const std::string& streamData);

    /// 解析 Office 97-2003 加密流（InputStream 版本，简化接口）
    /// @param stream 输入流
    /// @return IEncryption 实现
    static std::unique_ptr<IEncryption> ParseEncryption03(InputStream& stream);

    /// 解析 Office 2007+ EncryptionInfo 流
    /// @param xml XML 内容
    /// @param ctx 上下文（可选，用于错误报告）
    /// @return IEncryption 实现（EncryptionInfo）
    static std::unique_ptr<IEncryption> ParseEncryptionInfoStream(
        const std::string& xml, BaseContext* ctx = nullptr);
};

} // namespace crypto
} // namespace office
} // namespace zq
