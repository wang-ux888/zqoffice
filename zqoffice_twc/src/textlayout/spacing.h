// =============================================================================
// src/textlayout/spacing.h
//
// Spacing 类（间距）
//
//   段落间距属性，含段前/段后/行距，单位 px + 百分比：
//     - BeforePx / BeforeLinePercent / BeforeAutoSpacing
//     - AfterPx / AfterLinePercent / AfterAutoSpacing
//     - LinePx / LinePercent / LineGapPx / LineRule
// =============================================================================
#pragma once

#include "textlayout/textlayout_enums.h"

namespace zq {
namespace office {
namespace textlayout {

/// 段落间距
class Spacing {
public:
    Spacing() = default;
    ~Spacing() = default;

    // -----------------------------------------------------------------------
    // 段前
    // -----------------------------------------------------------------------

    float GetBeforePx() const { return beforePx_; }
    void SetBeforePx(float v) { beforePx_ = v; }

    float GetBeforeLinePercent() const { return beforeLinePercent_; }
    void SetBeforeLinePercent(float v) { beforeLinePercent_ = v; }

    bool IsBeforeAutoSpacing() const { return beforeAutoSpacing_; }
    void SetBeforeAutoSpacing(bool v) { beforeAutoSpacing_ = v; }

    // -----------------------------------------------------------------------
    // 段后
    // -----------------------------------------------------------------------

    float GetAfterPx() const { return afterPx_; }
    void SetAfterPx(float v) { afterPx_ = v; }

    float GetAfterLinePercent() const { return afterLinePercent_; }
    void SetAfterLinePercent(float v) { afterLinePercent_ = v; }

    bool IsAfterAutoSpacing() const { return afterAutoSpacing_; }
    void SetAfterAutoSpacing(bool v) { afterAutoSpacing_ = v; }

    // -----------------------------------------------------------------------
    // 行距
    // -----------------------------------------------------------------------

    float GetLinePx() const { return linePx_; }
    void SetLinePx(float v, RulerType /*rule*/) { linePx_ = v; }

    float GetLinePercent() const { return linePercent_; }
    void SetLinePercent(float v) { linePercent_ = v; }

    float GetLineGapPx() const { return lineGapPx_; }
    void SetLineGapPx(float v) { lineGapPx_ = v; }

    RulerType GetLineRule() const { return lineRule_; }
    void SetLineRule(RulerType r) { lineRule_ = r; }

private:
    // 段前
    float beforePx_ = 0.0f;
    float beforeLinePercent_ = 0.0f;
    bool  beforeAutoSpacing_ = false;

    // 段后
    float afterPx_ = 0.0f;
    float afterLinePercent_ = 0.0f;
    bool  afterAutoSpacing_ = false;

    // 行距
    float linePx_ = 0.0f;
    float linePercent_ = 0.0f;
    float lineGapPx_ = 0.0f;
    RulerType lineRule_ = RulerType::kAuto;
};

} // namespace textlayout
} // namespace office
} // namespace zq
