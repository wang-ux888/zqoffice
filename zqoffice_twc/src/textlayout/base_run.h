// =============================================================================
// src/textlayout/base_run.h
//
// BaseRun 类（Run 抽象基类）
//
//   Run 抽象基类，派生出 TextRun（文本 Run）和 ObjectRun（对象 Run）。
//
//   关键虚方法（纯虚）：
//     - Layout(IFontManager*)      : 布局
//     - GetAscent() / GetDescent() : 度量
//     - GetAdvances()              : 字符推进数组
//     - GetType()                  : Run 类型
//     - GetFontInfo()              : 字体信息
//
//   关键虚方法（非纯虚，有默认实现）：
//     - IsControlRun()             : 是否控制 Run（默认 false）
//     - Visible()                  : 是否可见（默认 true）
//     - GetCharCount()             : 字符数（默认 0）
//
//   非虚方法：
//     - GetRunPr() / GetStartCharPos() / GetEndCharPos()
//     - GetWidth() / GetHeight() / GetBreakType() / SetBreakType()
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include "textlayout/textlayout_enums.h"
#include "textlayout/font_info.h"

namespace zq {
namespace office {
namespace textlayout {

class Paragraph;
class RunPr;
class IFontManager;

/// Run 抽象基类
class BaseRun {
public:
    /// 构造
    /// @param para 所属段落
    /// @param runPr Run 属性（不转移所有权，BaseRun 持有原始指针）
    /// @param startCharPos 起始字符位置（段落内）
    /// @param charCount 字符数
    BaseRun(Paragraph* para, RunPr* runPr,
            std::uint32_t startCharPos, std::uint32_t charCount);

    virtual ~BaseRun() = default;

    // -----------------------------------------------------------------------
    // 纯虚方法（派生类必须实现）
    // -----------------------------------------------------------------------

    /// 布局（由 IFontManager 测量文本，填充度量数据）
    virtual void Layout(IFontManager* fontMgr) = 0;

    /// 字符 ascent（基线到顶部）
    virtual float GetAscent() const = 0;

    /// 字符 descent（基线到底部）
    virtual float GetDescent() const = 0;

    /// 字符推进数组（每字符水平推进宽度）
    /// @return {字符数, 推进数组指针}
    virtual std::pair<std::uint32_t, const float*> GetAdvances() const = 0;

    /// Run 类型
    virtual RunType GetType() const = 0;

    /// 字体信息
    virtual const FontInfo* GetFontInfo() const = 0;

    // -----------------------------------------------------------------------
    // 虚方法（有默认实现）
    // -----------------------------------------------------------------------

    /// 是否控制 Run（换行/分页/制表符等）
    virtual bool IsControlRun() const { return false; }

    /// 是否可见（控制 Run 不可见）
    virtual bool Visible() const { return true; }

    /// 字符数（可被派生类覆盖）
    virtual std::uint32_t GetCharCount() const { return charCount_; }

    // -----------------------------------------------------------------------
    // 非虚方法
    // -----------------------------------------------------------------------

    /// Run 属性
    RunPr* GetRunPr() const { return runPr_; }

    /// 起始字符位置（段落内）
    std::uint32_t GetStartCharPos() const { return startCharPos_; }

    /// 结束字符位置（不含，= startCharPos_ + charCount_）
    std::uint32_t GetEndCharPos() const { return startCharPos_ + charCount_; }

    /// 宽度（px）
    float GetWidth() const { return width_; }

    /// 高度（px）
    float GetHeight() const { return height_; }

    /// 换行类型
    LineBreakType GetBreakType() const { return breakType_; }

    /// 设置换行类型
    void SetBreakType(LineBreakType t) { breakType_ = t; }

    // -----------------------------------------------------------------------
    // 内部设置（供派生类/Paragraph 调用）
    // -----------------------------------------------------------------------

    /// 设置宽度
    void SetWidth(float w) { width_ = w; }

    /// 设置高度
    void SetHeight(float h) { height_ = h; }

    /// 设置起始字符位置（Paragraph::FormatRunList 时调整）
    void SetStartCharPos(std::uint32_t pos) { startCharPos_ = pos; }

    /// 设置字符数
    void SetCharCount(std::uint32_t cnt) { charCount_ = cnt; }

protected:
    Paragraph* paragraph_;              // 所属段落
    RunPr* runPr_;                      // Run 属性（非拥有）
    std::uint32_t startCharPos_;        // 起始字符位置
    std::uint32_t charCount_;           // 字符数
    float width_ = 0.0f;                // 宽度
    float height_ = 0.0f;               // 高度
    LineBreakType breakType_ = LineBreakType::kNone;  // 换行类型
};

} // namespace textlayout
} // namespace office
} // namespace zq
