// =============================================================================
// src/textlayout/font_info.h
//
// FontInfo 类（字体度量）
//
//   字体度量信息，含 ascent/descent/leading/top/bottom + 字体 ID。
//   用于 BaseRun::GetFontInfo() 返回值。
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {
namespace textlayout {

/// 字体度量
class FontInfo {
public:
    FontInfo() = default;

    FontInfo(float ascent, float descent, float leading,
             float top, float bottom, std::uint32_t fontId)
        : ascent_(ascent), descent_(descent), leading_(leading),
          top_(top), bottom_(bottom), fontId_(fontId) {}

    ~FontInfo() = default;

    // -----------------------------------------------------------------------
    // 度量值
    // -----------------------------------------------------------------------

    float GetAscent() const { return ascent_; }
    void SetAscent(float v) { ascent_ = v; }

    float GetDescent() const { return descent_; }
    void SetDescent(float v) { descent_ = v; }

    float GetLeading() const { return leading_; }
    void SetLeading(float v) { leading_ = v; }

    float GetTop() const { return top_; }
    void SetTop(float v) { top_ = v; }

    float GetBottom() const { return bottom_; }
    void SetBottom(float v) { bottom_ = v; }

    // -----------------------------------------------------------------------
    // 字体 ID
    // -----------------------------------------------------------------------

    std::uint32_t GetFontId() const { return fontId_; }
    void SetFontId(std::uint32_t id) { fontId_ = id; }

private:
    float ascent_ = 0.0f;       // 字符基线到顶部距离
    float descent_ = 0.0f;      // 字符基线到底部距离
    float leading_ = 0.0f;      // 行间距额外高度
    float top_ = 0.0f;          // 字符顶部（含 ascent 上方空间）
    float bottom_ = 0.0f;       // 字符底部（含 descent 下方空间）
    std::uint32_t fontId_ = 0;  // 字体 ID
};

} // namespace textlayout
} // namespace office
} // namespace zq
