// =============================================================================
// src/biff/ppt_record_header.h
//
// PowerPoint 记录头（[MS-PPT] 2.3 节）
//
//   PPT 记录结构：8 字节头 + N 字节数据
//     Offset 0: RecVer (4 bits) + RecInstance (12 bits)  → 2 字节
//     Offset 2: RecType (2 字节)
//     Offset 4: RecLen (4 字节)
//     Offset 8: Record Data (N 字节)
//
// 注意：PPT 记录可能嵌套（container），RecVer=0xF 表示容器
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>

namespace zq {
namespace office {
namespace biff {

/// PowerPoint 记录头（8 字节）
class PptRecordHeader {
public:
    using Type = std::uint16_t;

    PptRecordHeader() = default;

    /// 从 8 字节缓冲读取
    bool Read(const unsigned char* buf, std::size_t size);

    /// RecVer：记录版本（4 bits）
    /// 0xF = 容器（包含子记录），其他值 = 原子记录
    std::uint8_t recVer() const { return recVer_; }

    /// RecInstance：记录实例（12 bits）
    std::uint16_t recInstance() const { return recInstance_; }

    /// RecType：记录类型（2 字节，如 0x03F0 = Slide, 0x03E8 = Document）
    std::uint16_t recType() const { return recType_; }

    /// RecLen：数据长度（4 字节）
    std::uint32_t recLen() const { return recLen_; }

    /// 统一接口：记录类型（用于 RecordEnumerator 模板）
    std::uint16_t type() const { return recType_; }

    /// 统一接口：数据长度（用于 RecordEnumerator 模板）
    std::uint32_t length() const { return recLen_; }

    /// 是否为容器记录（RecVer == 0xF）
    bool isContainer() const { return recVer_ == 0x0F; }

    /// 是否为原子记录
    bool isAtom() const { return !isContainer(); }

    /// 头部大小（固定 8 字节）
    static constexpr std::size_t kHeaderSize = 8;

private:
    std::uint8_t  recVer_ = 0;
    std::uint16_t recInstance_ = 0;
    std::uint16_t recType_ = 0;
    std::uint32_t recLen_ = 0;
};

} // namespace biff
} // namespace office
} // namespace zq
