// =============================================================================
// src/textlayout/run_pr.h
//
// RunPr 类（Run 属性）
//
//   - 字号（GetTextSize/SetTextSize）
//   - 文本色/背景色（GetTextColor/SetTextColor/GetTextBGColor/SetTextBGColor）
//   - 字体名（含 ASCII/CJK/Hansi/Cs 区分，共 5 套 Get/Set）
//   - B/I/U/Strike/Outline/Shadow/Condense/Extend 加粗斜体下划线等
//   - 字符集/族（SetCharset/SetFamily）
//   - 字符垂直对齐（GetVerticalAlign/SetVerticalAlign）
//   - 相等比较（operator==）
//
//   颜色格式：0xRRGGBB（uint32_t，高字节为 0）
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "textlayout/textlayout_enums.h"

namespace zq {
namespace office {
namespace textlayout {

/// Run 属性
class RunPr {
public:
    RunPr() = default;
    ~RunPr() = default;

    RunPr(const RunPr&) = default;
    RunPr& operator=(const RunPr&) = default;
    RunPr(RunPr&&) noexcept = default;
    RunPr& operator=(RunPr&&) noexcept = default;

    // -----------------------------------------------------------------------
    // 字号
    // -----------------------------------------------------------------------

    float GetTextSize() const { return textSize_; }
    void SetTextSize(float sz) { textSize_ = sz; }

    // -----------------------------------------------------------------------
    // 文本色 / 背景色（0xRRGGBB）
    // -----------------------------------------------------------------------

    std::uint32_t GetTextColor() const { return textColor_; }
    void SetTextColor(std::uint32_t c) { textColor_ = c; }

    std::uint32_t GetTextBGColor() const { return textBGColor_; }
    void SetTextBGColor(std::uint32_t c) { textBGColor_ = c; }

    // -----------------------------------------------------------------------
    // 字体名（5 套：主/ASCII/CJK/Hansi/Cs）
    // -----------------------------------------------------------------------

    const std::string& GetFontName() const { return fontName_; }
    void SetFontName(const std::string& n) { fontName_ = n; }

    const std::string& GetAsciiFontName() const { return asciiFontName_; }
    void SetAsciiFontName(const std::string& n) { asciiFontName_ = n; }

    const std::string& GetCJKFontName() const { return cjkFontName_; }
    void SetCJKFontName(const std::string& n) { cjkFontName_ = n; }

    const std::string& GetHansiFontName() const { return hansiFontName_; }
    void SetHansiFontName(const std::string& n) { hansiFontName_ = n; }

    const std::string& GetCsFontName() const { return csFontName_; }
    void SetCsFontName(const std::string& n) { csFontName_ = n; }

    // -----------------------------------------------------------------------
    // B / I / U（加粗 / 斜体 / 下划线）
    // -----------------------------------------------------------------------

    bool IsB() const { return b_; }
    void SetB(bool v) { b_ = v; }

    bool IsI() const { return i_; }
    void SetI(bool v) { i_ = v; }

    bool IsU() const { return u_; }
    void SetU(bool v) { u_ = v; }

    // -----------------------------------------------------------------------
    // 其他修饰
    // -----------------------------------------------------------------------

    bool IsStrike() const { return strike_; }
    void SetStrike(bool v) { strike_ = v; }

    bool IsOutline() const { return outline_; }
    void SetOutline(bool v) { outline_ = v; }

    bool IsShadow() const { return shadow_; }
    void SetShadow(bool v) { shadow_ = v; }

    bool IsCondense() const { return condense_; }
    void SetCondense(bool v) { condense_ = v; }

    bool IsExtend() const { return extend_; }
    void SetExtend(bool v) { extend_ = v; }

    // -----------------------------------------------------------------------
    // 字符集 / 族
    // -----------------------------------------------------------------------

    int GetCharset() const { return charset_; }
    void SetCharset(int c) { charset_ = c; }

    int GetFamily() const { return family_; }
    void SetFamily(int f) { family_ = f; }

    // -----------------------------------------------------------------------
    // 字符垂直对齐
    // -----------------------------------------------------------------------

    CharacterVerticalAlignment GetVerticalAlign() const { return vertAlign_; }
    void SetVerticalAlign(CharacterVerticalAlignment a) { vertAlign_ = a; }

    // -----------------------------------------------------------------------
    // 唯一 ID（用于 Paragraph::EnableUniqueIDForEachRunPr）
    // -----------------------------------------------------------------------

    std::uint32_t GetUniqueId() const { return uniqueId_; }
    void SetUniqueId(std::uint32_t id) { uniqueId_ = id; }

    // -----------------------------------------------------------------------
    // 相等比较
    // -----------------------------------------------------------------------

    bool operator==(const RunPr& other) const {
        return textSize_ == other.textSize_ &&
               textColor_ == other.textColor_ &&
               textBGColor_ == other.textBGColor_ &&
               fontName_ == other.fontName_ &&
               asciiFontName_ == other.asciiFontName_ &&
               cjkFontName_ == other.cjkFontName_ &&
               hansiFontName_ == other.hansiFontName_ &&
               csFontName_ == other.csFontName_ &&
               b_ == other.b_ && i_ == other.i_ && u_ == other.u_ &&
               strike_ == other.strike_ && outline_ == other.outline_ &&
               shadow_ == other.shadow_ && condense_ == other.condense_ &&
               extend_ == other.extend_ && charset_ == other.charset_ &&
               family_ == other.family_ && vertAlign_ == other.vertAlign_;
    }

    bool operator!=(const RunPr& other) const { return !(*this == other); }

private:
    // 字号
    float textSize_ = 12.0f;        // 默认 12pt

    // 颜色（0xRRGGBB）
    std::uint32_t textColor_ = 0x000000u;   // 默认黑色
    std::uint32_t textBGColor_ = 0xFFFFFFu; // 默认白色

    // 字体名
    std::string fontName_;          // 主字体名
    std::string asciiFontName_;     // ASCII 字体
    std::string cjkFontName_;       // CJK 字体
    std::string hansiFontName_;     // Hansi 字体
    std::string csFontName_;        // Complex Script 字体

    // 修饰
    bool b_ = false;        // 加粗
    bool i_ = false;        // 斜体
    bool u_ = false;        // 下划线
    bool strike_ = false;   // 删除线
    bool outline_ = false;  // 轮廓
    bool shadow_ = false;   // 阴影
    bool condense_ = false; // 压缩
    bool extend_ = false;   // 扩展

    // 字符集/族
    int charset_ = 0;       // 字符集
    int family_ = 0;        // 字体族

    // 字符垂直对齐
    CharacterVerticalAlignment vertAlign_ = CharacterVerticalAlignment::kBaseline;

    // 唯一 ID（用于 Paragraph::EnableUniqueIDForEachRunPr）
    std::uint32_t uniqueId_ = 0;
};

} // namespace textlayout
} // namespace office
} // namespace zq
