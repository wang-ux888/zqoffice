// =============================================================================
// src/escher/bse_record.h
//
// Blip Store Entry（BSE）记录（[MS-ODRAW] 2.2.40 OfficeArtBStoreContainerEntry）
//
//   BSE 记录包含图像元信息 + 嵌入的 BLIP 记录（实际图像数据）。
//
//   BSE 记录格式：
//     Header (8 字节，recType=0xF007, recVer=0x2, recInstance=blipType)
//     btWin32   (1 字节)  Windows 平台 BLIP 类型
//     btMacOS   (1 字节)  Mac 平台 BLIP 类型
//     uid       (16 字节) 唯一标识符（GUID）
//     tag       (2 字节)  标记（通常 0xFF）
//     size      (4 字节)  BLIP 在流中的总大小
//     refcount  (4 字节)  引用计数
//     unused1   (4 字节)  未使用
//     unused2   (4 字节)  未使用
//     unused3   (4 字节)  未使用
//     blip      (变长)    嵌入的 BLIP 记录
//
//   固定部分（不含 Escher 头）= 1+1+16+2+4+4+4+4+4 = 40 字节
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>

#include "escher_record.h"

namespace zq {
namespace office {
namespace escher {

/// Blip Store Entry（BSE）记录
class BSERecord {
public:
    /// BSE 固定部分大小（不含 Escher 头）
    static constexpr std::size_t kBSEBodySize = 40;

    BSERecord() = default;

    /// 从缓冲解析 BSE 记录
    /// @param buf  数据缓冲（应包含 Escher 头 + BSE 体 + BLIP）
    /// @param size 缓冲字节数
    /// @return 解析成功返回 true
    bool Read(const unsigned char* buf, std::size_t size);

    /// Windows 平台 BLIP 类型
    std::uint8_t btWin32() const { return btWin32_; }

    /// Mac 平台 BLIP 类型
    std::uint8_t btMacOS() const { return btMacOS_; }

    /// 唯一标识符（16 字节 GUID）
    const unsigned char* uid() const { return uid_; }

    /// 标记（通常 0xFF）
    std::uint16_t tag() const { return tag_; }

    /// BLIP 在流中的总大小
    std::uint32_t size() const { return size_; }

    /// 引用计数
    std::uint32_t refcount() const { return refcount_; }

    /// BLIP 类型（与 btWin32 一致，用于统一接口）
    std::uint16_t blipType() const { return btWin32_; }

    /// 嵌入的 BLIP 记录数据指针（含 BLIP 自身的 Escher 头）
    const unsigned char* blipData() const { return blipData_; }

    /// 嵌入的 BLIP 记录数据大小
    std::size_t blipSize() const { return blipSize_; }

    /// BLIP 类型字符串名称
    const char* blipTypeName() const { return escher::blipTypeName(btWin32_); }

private:
    std::uint8_t  btWin32_ = 0;
    std::uint8_t  btMacOS_ = 0;
    unsigned char uid_[16] = {0};
    std::uint16_t tag_ = 0;
    std::uint32_t size_ = 0;
    std::uint32_t refcount_ = 0;

    const unsigned char* blipData_ = nullptr;
    std::size_t blipSize_ = 0;
};

} // namespace escher
} // namespace office
} // namespace zq
