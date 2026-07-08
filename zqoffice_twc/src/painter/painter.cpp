// =============================================================================
// src/painter/painter.cpp
//
// Painter 类实现
// =============================================================================
#include "painter/painter.h"

namespace zq {
namespace office {
namespace painter {

Painter::Painter()
    : color_(0xFF000000)  // 不透明黑色
    , bold_(false)
    , italic_(false)
    , underline_(false)
    , text_size_(12.0f)
    , stroke_width_(1.0f)
    , stroke_miter_(4.0f)  // Skia 默认 miter = 4
    , font_family_()
    , font_id_(0)
    , cap_(Cap::kButt)
    , join_(Join::kMiter)
    , style_(Style::kFill) {}

// ----- Get 系列 -----

uint8_t Painter::GetAlpha() const {
    return static_cast<uint8_t>((color_ >> 24) & 0xFF);
}

uint8_t Painter::GetRed() const {
    return static_cast<uint8_t>((color_ >> 16) & 0xFF);
}

uint8_t Painter::GetGreen() const {
    return static_cast<uint8_t>((color_ >> 8) & 0xFF);
}

uint8_t Painter::GetBlue() const {
    return static_cast<uint8_t>(color_ & 0xFF);
}

uint32_t Painter::GetColor() const {
    return color_;
}

bool Painter::IsBold() const { return bold_; }
bool Painter::IsItalic() const { return italic_; }
bool Painter::IsUnderLine() const { return underline_; }

float Painter::GetTextSize() const { return text_size_; }
float Painter::GetStrokeWidth() const { return stroke_width_; }
float Painter::GetStrokeMiter() const { return stroke_miter_; }

const std::string& Painter::GetFontFamily() const { return font_family_; }
size_t Painter::GetFontId() const { return font_id_; }

Cap Painter::GetCap() const { return cap_; }
Join Painter::GetJoin() const { return join_; }
Style Painter::GetStyle() const { return style_; }

// ----- Set 系列 -----

void Painter::SetColor(uint32_t color) { color_ = color; }

void Painter::SetAlpha(uint8_t alpha) {
    color_ = (color_ & 0x00FFFFFF) | (static_cast<uint32_t>(alpha) << 24);
}

void Painter::SetRed(uint8_t red) {
    color_ = (color_ & 0xFF00FFFF) | (static_cast<uint32_t>(red) << 16);
}

void Painter::SetGreen(uint8_t green) {
    color_ = (color_ & 0xFFFF00FF) | (static_cast<uint32_t>(green) << 8);
}

void Painter::SetBlue(uint8_t blue) {
    color_ = (color_ & 0xFFFFFF00) | static_cast<uint32_t>(blue);
}

void Painter::SetBold(bool bold) { bold_ = bold; }
void Painter::SetItalic(bool italic) { italic_ = italic; }
void Painter::SetUnderLine(bool underline) { underline_ = underline; }

void Painter::SetTextSize(float size) { text_size_ = size; }
void Painter::SetStrokeWidth(float width) { stroke_width_ = width; }
void Painter::SetStrokeMiter(float miter) { stroke_miter_ = miter; }

void Painter::SetFontFamily(const std::string& family) { font_family_ = family; }
void Painter::SetFontId(size_t id) { font_id_ = id; }

void Painter::SetCap(Cap cap) { cap_ = cap; }
void Painter::SetJoin(Join join) { join_ = join; }
void Painter::SetStyle(Style style) { style_ = style; }

}  // namespace painter
}  // namespace office
}  // namespace zq
