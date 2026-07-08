// =============================================================================
// src/biff/biff_record_header.h
//
// BIFF 记录头（[MS-XLS] 2.4 节）
//
//   BIFF 记录结构：4 字节头 + N 字节数据
//     Offset 0: Record Type (2 字节，操作码)
//     Offset 2: Record Length (2 字节，数据长度)
//     Offset 4: Record Data (N 字节)
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>

namespace zq {
namespace office {
namespace biff {

/// BIFF 记录头（4 字节）
class BIFFRecordHeader {
public:
    using Type = std::uint16_t;

    BIFFRecordHeader() = default;
    BIFFRecordHeader(std::uint16_t type, std::uint16_t length)
        : type_(type), length_(length) {}

    /// 从 4 字节缓冲读取
    /// @return 解析成功返回 true
    bool Read(const unsigned char* buf, std::size_t size);

    /// 记录类型（操作码，如 0x0085 = BoundSheet8, 0x0203 = Number）
    std::uint16_t type() const { return type_; }

    /// 数据长度（字节数）
    std::uint16_t length() const { return length_; }

    /// 头部大小（固定 4 字节）
    static constexpr std::size_t kHeaderSize = 4;

private:
    std::uint16_t type_ = 0;
    std::uint16_t length_ = 0;
};

} // namespace biff
} // namespace office
} // namespace zq
