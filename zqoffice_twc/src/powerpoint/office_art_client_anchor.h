// =============================================================================
// src/powerpoint/office_art_client_anchor.h
//
// PowerPoint 客户端锚点记录（[MS-ODRAW] 2.4.9 + [MS-PPT] 2.3.4）
//
//   继承自 EscherClientAnchorRecord（即 escher::ClientAnchorRecord）。
//
//   PowerPoint 锚点格式（8 字节，4×int16）：
//     Offset 0: top    (int16, 1/8 point 单位)
//     Offset 2: left   (int16, 1/8 point 单位)
//     Offset 4: right  (int16, 1/8 point 单位)
//     Offset 6: bottom (int16, 1/8 point 单位)
//
//   GetL/GetT/GetR/GetB 返回 EMU 单位值：
//     1 point = 12700 EMU
//     1/8 point = 12700/8 = 1587.5 EMU
//     EMU = raw_value * 12700 / 8
//
// 导出符号对齐：
//   - 构造（默认/移动/拷贝）
//   - virtual ~OfficeArtClientAnchor()
//   - virtual int getRecordSize()
//   - virtual std::string getRecordName() const
//   - virtual int fillFields(...)
//   - int GetL/GetT/GetR/GetB() const  (PPT 锚点四元组，单位 EMU)
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "escher/client_anchor_record.h"

namespace zq {
namespace office {
namespace powerpoint {

/// PowerPoint 客户端锚点记录
///
///   - 继承 escher::ClientAnchorRecord（即 EscherClientAnchorRecord）
///   - PPT 锚点格式：4×int16（top/left/right/bottom，1/8 point 单位）
///   - GetL/GetT/GetR/GetB 返回 EMU 单位值
///
/// 注意：基类 ClientAnchorRecord 持有视图指针（raw_），不拥有数据。
/// 本派生类在 fillFields 中将 PPT 锚点 int16 值拷贝至成员变量，
/// 使 GetL/GetT/GetR/GetB 不依赖基类指针，支持安全的拷贝/移动。
class OfficeArtClientAnchor : public escher::ClientAnchorRecord {
public:
    /// PPT 锚点转换常量：1/8 point → EMU
    /// 1 point = 12700 EMU，1/8 point = 12700/8 = 1587.5 EMU
    /// 使用整数运算避免浮点精度损失：EMU = raw * 12700 / 8
    static constexpr std::int32_t kPointToEmu = 12700;
    static constexpr std::int32_t kPointEighthDivisor = 8;

    OfficeArtClientAnchor() = default;

    /// 移动构造（默认）
    OfficeArtClientAnchor(OfficeArtClientAnchor&&) noexcept = default;
    OfficeArtClientAnchor& operator=(OfficeArtClientAnchor&&) noexcept = default;

    /// 拷贝构造（默认；成员均为值类型，可安全拷贝）
    OfficeArtClientAnchor(const OfficeArtClientAnchor&) = default;
    OfficeArtClientAnchor& operator=(const OfficeArtClientAnchor&) = default;

    ~OfficeArtClientAnchor() override = default;

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    /// 记录数据大小（PPT 锚点固定 8 字节）
    virtual int getRecordSize();

    /// 记录名称
    virtual std::string getRecordName() const { return "OfficeArtClientAnchor"; }

    /// 解析记录数据
    /// @param data 记录数据指针（不含 Escher 头部，纯 8 字节 PPT 锚点）
    /// @param size 数据字节数
    /// @return 实际消费的字节数，<0 表示错误
    virtual int fillFields(const unsigned char* data, int size);

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    /// Left（EMU）
    std::int32_t GetL() const {
        return static_cast<std::int32_t>(left_) * kPointToEmu / kPointEighthDivisor;
    }

    /// Top（EMU）
    std::int32_t GetT() const {
        return static_cast<std::int32_t>(top_) * kPointToEmu / kPointEighthDivisor;
    }

    /// Right（EMU）
    std::int32_t GetR() const {
        return static_cast<std::int32_t>(right_) * kPointToEmu / kPointEighthDivisor;
    }

    /// Bottom（EMU）
    std::int32_t GetB() const {
        return static_cast<std::int32_t>(bottom_) * kPointToEmu / kPointEighthDivisor;
    }

    /// 宽度（Right - Left，EMU）
    std::int32_t GetWidth() const { return GetR() - GetL(); }

    /// 高度（Bottom - Top，EMU）
    std::int32_t GetHeight() const { return GetB() - GetT(); }

private:
    /// PPT 锚点原始值（1/8 point 单位，由 fillFields 解析）
    std::int16_t top_ = 0;
    std::int16_t left_ = 0;
    std::int16_t right_ = 0;
    std::int16_t bottom_ = 0;
};

} // namespace powerpoint
} // namespace office
} // namespace zq
