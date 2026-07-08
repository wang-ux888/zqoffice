// =============================================================================
// src/painter/lynx/lynx_font_metrics.h
//
// zq::office::painter::lynx::LynxFontMetrics 结构体
//   下划线/删除线的位置与粗细查询（用于文本绘制时的装饰线绘制）
//
// =============================================================================
#pragma once

namespace zq {
namespace office {
namespace painter {
namespace lynx {

/// 字体度量信息（Lynx 渲染框架适配）
/// 用于文本测量时获取下划线/删除线几何信息
struct LynxFontMetrics {
    // 下划线
    bool   has_underline_position;   ///< 是否有下划线位置
    bool   has_underline_thickness;  ///< 是否有下划线粗细
    float  underline_position;       ///< 下划线位置（基线到下划线中心的距离）
    float  underline_thickness;      ///< 下划线粗细

    // 删除线
    bool   has_strikeout_position;   ///< 是否有删除线位置
    bool   has_strikeout_thickness;  ///< 是否有删除线粗细
    float  strikeout_position;       ///< 删除线位置
    float  strikeout_thickness;      ///< 删除线粗细

    /// 默认构造：全部无下划线/删除线信息
    LynxFontMetrics();

    /// 拷贝构造
    LynxFontMetrics(const LynxFontMetrics& other) = default;

    /// 移动构造
    LynxFontMetrics(LynxFontMetrics&& other) noexcept = default;

    /// 析构
    ~LynxFontMetrics() = default;

    /// 拷贝赋值
    LynxFontMetrics& operator=(const LynxFontMetrics& other) = default;

    /// 移动赋值
    LynxFontMetrics& operator=(LynxFontMetrics&& other) noexcept = default;


    /// 查询下划线位置
    /// @param out  输出参数：下划线位置
    /// @return true 表示有下划线位置信息
    bool hasUnderlinePosition(float* out) const;

    /// 查询下划线粗细
    /// @param out  输出参数：下划线粗细
    /// @return true 表示有下划线粗细信息
    bool hasUnderlineThickness(float* out) const;

    /// 查询删除线位置
    /// @param out  输出参数：删除线位置
    /// @return true 表示有删除线位置信息
    bool hasStrikeoutPosition(float* out) const;

    /// 查询删除线粗细
    /// @param out  输出参数：删除线粗细
    /// @return true 表示有删除线粗细信息
    bool hasStrikeoutThickness(float* out) const;
};

}  // namespace lynx
}  // namespace painter
}  // namespace office
}  // namespace zq
