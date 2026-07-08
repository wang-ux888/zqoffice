// =============================================================================
// src/painter/painter.h
//
// zq::office::painter::Painter 类
//   封装绘制状态（ARGB 颜色、字体属性 B/I/U、字号、描边、字体族等）
//   约 30 个 Get/Set 方法 + 3 个枚举（Cap/Join/Style）
//
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "painter/painter_enums.h"

namespace zq {
namespace office {
namespace painter {

/// 绘制状态/画笔类
/// 封装绘制时的颜色、字体、描边等状态
class Painter {
public:
    /// 默认构造：黑色、无加粗/斜体/下划线、字号 12、描边宽 1.0
    Painter();

    /// 拷贝构造
    Painter(const Painter& other) = default;

    /// 移动构造
    Painter(Painter&& other) noexcept = default;

    /// 析构
    ~Painter() = default;

    /// 拷贝赋值
    Painter& operator=(const Painter& other) = default;

    /// 移动赋值
    Painter& operator=(Painter&& other) noexcept = default;

    // ----- Get 系列 -----

    /// 取 Alpha 通道 [0, 255]
    uint8_t GetAlpha() const;
    /// 取 Red 通道 [0, 255]
    uint8_t GetRed() const;
    /// 取 Green 通道 [0, 255]
    uint8_t GetGreen() const;
    /// 取 Blue 通道 [0, 255]
    uint8_t GetBlue() const;
    /// 取整体颜色（ARGB，高 8 位 Alpha，低 24 位 RGB）
    uint32_t GetColor() const;

    /// 是否加粗
    bool IsBold() const;
    /// 是否斜体
    bool IsItalic() const;
    /// 是否下划线
    bool IsUnderLine() const;

    /// 取字号
    float GetTextSize() const;
    /// 取描边宽度
    float GetStrokeWidth() const;
    /// 取描边 miter（尖角限制）
    float GetStrokeMiter() const;

    /// 取字体族
    const std::string& GetFontFamily() const;
    /// 取字体 ID
    size_t GetFontId() const;

    /// 取线帽枚举
    Cap GetCap() const;
    /// 取连接枚举
    Join GetJoin() const;
    /// 取样式枚举
    Style GetStyle() const;

    // ----- Set 系列 -----

    /// 设置整体颜色（ARGB）
    void SetColor(uint32_t color);
    /// 设置 Alpha 通道
    void SetAlpha(uint8_t alpha);
    /// 设置 Red 通道
    void SetRed(uint8_t red);
    /// 设置 Green 通道
    void SetGreen(uint8_t green);
    /// 设置 Blue 通道
    void SetBlue(uint8_t blue);

    /// 设置加粗
    void SetBold(bool bold);
    /// 设置斜体
    void SetItalic(bool italic);
    /// 设置下划线
    void SetUnderLine(bool underline);

    /// 设置字号
    void SetTextSize(float size);
    /// 设置描边宽度
    void SetStrokeWidth(float width);
    /// 设置描边 miter
    void SetStrokeMiter(float miter);

    /// 设置字体族
    void SetFontFamily(const std::string& family);
    /// 设置字体 ID
    void SetFontId(size_t id);

    /// 设置线帽枚举
    void SetCap(Cap cap);
    /// 设置连接枚举
    void SetJoin(Join join);
    /// 设置样式枚举
    void SetStyle(Style style);

private:
    // ARGB 颜色（高 8 位 Alpha，依次 R/G/B）
    uint32_t color_;

    // 字体属性
    bool bold_;
    bool italic_;
    bool underline_;

    // 字号与描边
    float text_size_;
    float stroke_width_;
    float stroke_miter_;

    // 字体
    std::string font_family_;
    size_t font_id_;

    // 绘制枚举
    Cap cap_;
    Join join_;
    Style style_;
};

}  // namespace painter
}  // namespace office
}  // namespace zq
