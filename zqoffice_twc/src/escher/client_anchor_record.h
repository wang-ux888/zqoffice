// =============================================================================
// src/escher/client_anchor_record.h
//
// ClientAnchor 记录（[MS-ODRAW] 2.2.44 OfficeArtClientAnchorRecord）
//
//   ClientAnchor 记录体由客户端（Excel/PowerPoint/Word）自定义，用于定位形状。
//
//   常见锚点格式：
//     PowerPoint: 8 字节（top/left/right/bottom 各 2 字节，1/8 点单位）
//     Excel: 18 字节（col/colOff/row/rowOff 等结构，单位 EMU/像素）
//     Word:  类似 Excel
//
//   本类提供原始数据访问 + 通用 4×int16 PowerPoint 锚点解析。
//   Excel/Word 自定义锚点由上层通过 raw() 自行解析。
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>

#include "escher_record.h"

namespace zq {
namespace office {
namespace escher {

/// ClientAnchor 记录
class ClientAnchorRecord {
public:
    /// PowerPoint 锚点大小（4×int16）
    static constexpr std::size_t kPPTAnchorSize = 8;

    ClientAnchorRecord() = default;

    /// 虚析构：支持派生类（如 powerpoint::OfficeArtClientAnchor）安全继承
    virtual ~ClientAnchorRecord() = default;

    /// 从缓冲解析 ClientAnchor 记录
    /// @return 解析成功返回 true
    bool Read(const unsigned char* buf, std::size_t size);

    /// 原始锚点数据（客户端自定义格式）
    const unsigned char* raw() const { return raw_; }

    /// 原始锚点数据大小
    std::size_t rawSize() const { return rawSize_; }

    // --- PowerPoint 4×int16 锚点（仅当 rawSize >= 8 时有效） ---

    std::int16_t top() const { return top_; }
    std::int16_t left() const { return left_; }
    std::int16_t right() const { return right_; }
    std::int16_t bottom() const { return bottom_; }

    /// 宽度（right - left）
    std::int32_t width() const { return right_ - left_; }

    /// 高度（bottom - top）
    std::int32_t height() const { return bottom_ - top_; }

private:
    const unsigned char* raw_ = nullptr;
    std::size_t rawSize_ = 0;

    std::int16_t top_ = 0;
    std::int16_t left_ = 0;
    std::int16_t right_ = 0;
    std::int16_t bottom_ = 0;
};

} // namespace escher
} // namespace office
} // namespace zq
