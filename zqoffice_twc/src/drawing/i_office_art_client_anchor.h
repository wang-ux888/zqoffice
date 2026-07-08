// =============================================================================
// src/drawing/i_office_art_client_anchor.h
//
// IOfficeArtClientAnchor 接口
//
//   客户端锚点接口，由 PPT/Excel 等具体客户端实现，用于从 Escher ClientAnchor
//   记录中解析出形状在文档中的位置和尺寸。
//
//   对应 [MS-ODRAW] 2.4.9 OfficeArtClientAnchor 记录（recType=0xF009）
//   实际记录格式由客户端定义（PPT 使用 8 字节固定结构，Excel 使用 cell 引用）。
//
// 导出符号对齐：
//   - ??0IOfficeArtClientAnchor (构造/拷贝)
//   - ??1...UEAA@XZ (virtual ~)
//   - ??4... (operator=)
//   - ??_7... (vtable)
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace zq {
namespace office {
namespace drawing {

class ShapePr;

/// OfficeArt 客户端锚点接口
///
/// 由具体客户端（如 powerpoint::OfficeArtClientAnchor）实现，
/// EscherDrawingConvert::ConvertClientAnchorRecord 通过此接口回调客户端解析逻辑。
class IOfficeArtClientAnchor {
public:
    virtual ~IOfficeArtClientAnchor() = default;

    /// 记录 ID（Escher recType）
    virtual std::uint16_t getRecordId() const = 0;

    /// 记录名称
    virtual std::string getRecordName() const = 0;

    /// 记录数据大小（不含头部）
    virtual int getRecordSize() = 0;

    /// 解析记录数据
    /// @param data 记录数据指针（不含头部）
    /// @param size 数据字节数
    /// @return 实际消费的字节数，<0 表示错误
    virtual int fillFields(const unsigned char* data, int size) = 0;

    /// 应用锚点到 ShapePr（设置 X/Y/W/H）
    /// @param shapePr 形状属性（输出）
    /// @param scale 缩放因子（用于组合形状的相对坐标）
    virtual void applyToShapePr(ShapePr* shapePr, float scale) = 0;
};

} // namespace drawing
} // namespace office
} // namespace zq
