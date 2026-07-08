// =============================================================================
// src/textlayout/run_metrics.h
//
// RunMetrics 类（Run 度量数据）
//
//   TextRun::Layout() 的输出，含：
//     - 字符推进数组（advances_）
//     - ascent / descent
//     - FontInfo
//   通过 SetRunMetrics(unique_ptr<RunMetrics>) 注入 TextRun。
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "textlayout/font_info.h"

namespace zq {
namespace office {
namespace textlayout {

/// Run 度量数据
class RunMetrics {
public:
    RunMetrics() = default;
    ~RunMetrics() = default;

    // -----------------------------------------------------------------------
    // 字符推进数组
    // -----------------------------------------------------------------------

    /// 设置字符推进数组（每个字符的水平推进宽度，px）
    void SetAdvances(const std::vector<float>& a) { advances_ = a; }
    void SetAdvances(std::vector<float>&& a) { advances_ = std::move(a); }

    /// 获取字符推进数组
    const std::vector<float>& GetAdvances() const { return advances_; }
    std::vector<float>& GetAdvances() { return advances_; }

    /// 字符数（= advances_.size()）
    std::uint32_t GetCharCount() const { return static_cast<std::uint32_t>(advances_.size()); }

    /// 总推进宽度（px）
    float GetTotalWidth() const {
        float total = 0.0f;
        for (float v : advances_) {
            total += v;
        }
        return total;
    }

    // -----------------------------------------------------------------------
    // 字体度量
    // -----------------------------------------------------------------------

    float GetAscent() const { return ascent_; }
    void SetAscent(float v) { ascent_ = v; }

    float GetDescent() const { return descent_; }
    void SetDescent(float v) { descent_ = v; }

    // -----------------------------------------------------------------------
    // FontInfo
    // -----------------------------------------------------------------------

    const FontInfo* GetFontInfo() const { return fontInfo_.get(); }
    void SetFontInfo(std::unique_ptr<FontInfo> fi) { fontInfo_ = std::move(fi); }

private:
    std::vector<float> advances_;   // 每字符水平推进宽度
    float ascent_ = 0.0f;          // 字符 ascent
    float descent_ = 0.0f;         // 字符 descent
    std::unique_ptr<FontInfo> fontInfo_;
};

} // namespace textlayout
} // namespace office
} // namespace zq
