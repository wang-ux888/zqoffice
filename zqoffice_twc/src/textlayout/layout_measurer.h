// =============================================================================
// src/textlayout/layout_measurer.h
//
// LayoutMeasurer 接口（布局测量器）
//
//   布局测量器，封装 IFontManager 与段落/页面上下文，提供：
//     - 字体加载与文本测量
//     - 行高计算（基于 RulerType 与 ascent/descent）
//     - 空格宽度测量（用于 justify 对齐）
//
//   TextLine 构造时持有 LayoutMeasurer 指针，用于在布局过程中测量文本。
//
//   本阶段仅定义接口，具体实现由后续 Phase（渲染层）提供。
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {
namespace textlayout {

class IFontManager;
class RunPr;
class PPr;

/// 布局测量器接口
class LayoutMeasurer {
public:
    virtual ~LayoutMeasurer() = default;

    /// 获取字体管理器
    virtual IFontManager* GetFontManager() = 0;

    /// 测量空格宽度（px）
    /// @param runPr Run 属性（决定字体）
    /// @return 空格字符的推进宽度
    virtual float MeasureSpaceWidth(const RunPr* runPr) = 0;

    /// 计算行高（px）
    /// @param ascent  最大 ascent
    /// @param descent 最大 descent
    /// @param pPr     段落属性（含行距规则）
    /// @return 行高
    virtual float CalculateLineHeight(float ascent, float descent,
                                       const PPr* pPr) = 0;
};

} // namespace textlayout
} // namespace office
} // namespace zq
