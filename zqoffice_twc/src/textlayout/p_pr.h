// =============================================================================
// src/textlayout/p_pr.h
//
// PPr 类（段落属性）
//
//   段落属性集合，含：
//     - 水平/垂直对齐
//     - 缩进（Indent）
//     - 间距（Spacing）
//     - 项目编号（NumPr）
//     - 制表符（Tabs）
//     - 默认 RunPr
//     - 末行对齐标志
//
//   所有权：Indent/Spacing/NumPr/Tabs/RunPr 均通过 unique_ptr 持有，
//   SetXxx 转移所有权，GetXxx 返回原始指针。
// =============================================================================
#pragma once

#include <memory>

#include "textlayout/textlayout_enums.h"
#include "textlayout/indent.h"
#include "textlayout/spacing.h"
#include "textlayout/tabs.h"
#include "textlayout/num_pr.h"

namespace zq {
namespace office {
namespace textlayout {

class RunPr;

/// 段落属性
class PPr {
public:
    PPr();
    ~PPr();

    PPr(PPr&&) noexcept = default;
    PPr& operator=(PPr&&) noexcept = default;

    // 禁止拷贝（持有 unique_ptr）
    PPr(const PPr&) = delete;
    PPr& operator=(const PPr&) = delete;

    // -----------------------------------------------------------------------
    // 对齐
    // -----------------------------------------------------------------------

    ParagraphHorizontalAlignment GetHorizontalAlign() const { return hAlign_; }
    void SetHorizontalAlign(ParagraphHorizontalAlignment a) { hAlign_ = a; }

    ParagraphVerticalAlignment GetVerticalAlign() const { return vAlign_; }
    void SetVerticalAlign(ParagraphVerticalAlignment a) { vAlign_ = a; }

    // -----------------------------------------------------------------------
    // 缩进
    // -----------------------------------------------------------------------

    Indent* GetIndent() const { return indent_.get(); }
    void SetIndent(std::unique_ptr<Indent> i) { indent_ = std::move(i); }

    // -----------------------------------------------------------------------
    // 间距
    // -----------------------------------------------------------------------

    Spacing* GetSpacing() const { return spacing_.get(); }
    void SetSpacing(std::unique_ptr<Spacing> s) { spacing_ = std::move(s); }

    // -----------------------------------------------------------------------
    // 项目编号
    // -----------------------------------------------------------------------

    NumPr* GetNumPr() const { return numPr_.get(); }
    void SetNumPr(std::unique_ptr<NumPr> n) { numPr_ = std::move(n); }

    // -----------------------------------------------------------------------
    // 制表符
    // -----------------------------------------------------------------------

    Tabs* GetTabs() const { return tabs_.get(); }
    void SetTabs(std::unique_ptr<Tabs> t) { tabs_ = std::move(t); }

    // -----------------------------------------------------------------------
    // 默认 RunPr
    // -----------------------------------------------------------------------

    RunPr* GetDefaultRunPr() const { return defaultRunPr_.get(); }
    void SetDefaultRunPr(std::unique_ptr<RunPr> r);

    // -----------------------------------------------------------------------
    // 末行对齐
    // -----------------------------------------------------------------------

    bool IsLastLineFollowHorizontalAlignment() const { return lastLineFollowHAlign_; }
    void SetLastLineFollowHorizontalAlignment(bool v) { lastLineFollowHAlign_ = v; }

private:
    ParagraphHorizontalAlignment hAlign_ = ParagraphHorizontalAlignment::kLeft;
    ParagraphVerticalAlignment vAlign_ = ParagraphVerticalAlignment::kTop;

    std::unique_ptr<Indent> indent_;
    std::unique_ptr<Spacing> spacing_;
    std::unique_ptr<NumPr> numPr_;
    std::unique_ptr<Tabs> tabs_;
    std::unique_ptr<RunPr> defaultRunPr_;

    bool lastLineFollowHAlign_ = false;
};

} // namespace textlayout
} // namespace office
} // namespace zq
