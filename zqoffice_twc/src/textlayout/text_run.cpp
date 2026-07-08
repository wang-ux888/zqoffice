// =============================================================================
// src/textlayout/text_run.cpp
//
// TextRun 类实现
// =============================================================================
#include "text_run.h"

#include "i_font_manager.h"
#include "run_pr.h"

namespace zq {
namespace office {
namespace textlayout {

TextRun::TextRun(Paragraph* para, RunPr* runPr,
                 std::uint32_t startCharPos, std::uint32_t charCount,
                 const std::string& text)
    : BaseRun(para, runPr, startCharPos, charCount)
    , text_(text) {}

void TextRun::Layout(IFontManager* fontMgr) {
    if (!fontMgr || !runPr_) {
        return;
    }

    // 1. 加载字体
    std::uint32_t fontId = fontMgr->LoadFontFromRunPr(runPr_);
    if (fontId == 0) {
        return;
    }

    // 2. 测量文本
    auto metrics = std::make_unique<RunMetrics>();
    std::vector<float> advances;
    float ascent = 0.0f, descent = 0.0f;

    if (fontMgr->MeasureText(fontId, text_.c_str(),
                              static_cast<std::uint32_t>(text_.size()),
                              advances, ascent, descent)) {
        metrics->SetAdvances(std::move(advances));
        metrics->SetAscent(ascent);
        metrics->SetDescent(descent);

        // 设置 FontInfo
        const FontInfo* fi = fontMgr->GetFontInfo(fontId);
        if (fi) {
            auto fiCopy = std::make_unique<FontInfo>(
                fi->GetAscent(), fi->GetDescent(), fi->GetLeading(),
                fi->GetTop(), fi->GetBottom(), fi->GetFontId());
            metrics->SetFontInfo(std::move(fiCopy));
        }

        // 更新 BaseRun 度量
        SetWidth(metrics->GetTotalWidth());
        SetHeight(ascent + descent);
    }

    runMetrics_ = std::move(metrics);
}

float TextRun::GetAscent() const {
    return runMetrics_ ? runMetrics_->GetAscent() : 0.0f;
}

float TextRun::GetDescent() const {
    return runMetrics_ ? runMetrics_->GetDescent() : 0.0f;
}

std::pair<std::uint32_t, const float*> TextRun::GetAdvances() const {
    if (runMetrics_) {
        const auto& adv = runMetrics_->GetAdvances();
        return { static_cast<std::uint32_t>(adv.size()), adv.data() };
    }
    return { 0, nullptr };
}

const FontInfo* TextRun::GetFontInfo() const {
    return runMetrics_ ? runMetrics_->GetFontInfo() : nullptr;
}

void TextRun::SetRunMetrics(std::unique_ptr<RunMetrics>& metrics) {
    runMetrics_ = std::move(metrics);
}

} // namespace textlayout
} // namespace office
} // namespace zq
