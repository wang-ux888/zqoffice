// =============================================================================
// src/textlayout/text_run.h
//
// TextRun 类（文本 Run）
//
//   文本 Run，持有文本内容 + RunMetrics（Layout 后填充）。
//
//   Layout(IFontManager*) 流程：
//     1. 通过 RunPr 获取字体名/字号，调用 IFontManager::LoadFont
//     2. 调用 IFontManager::MeasureText 测量文本
//     3. 将结果存入 RunMetrics
//     4. 设置 width_/height_/ascent_/descent_
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "textlayout/base_run.h"
#include "textlayout/run_metrics.h"

namespace zq {
namespace office {
namespace textlayout {

class Paragraph;
class RunPr;
class IFontManager;

/// 文本 Run
class TextRun : public BaseRun {
public:
    /// 构造
    /// @param para 所属段落
    /// @param runPr Run 属性
    /// @param startCharPos 起始字符位置
    /// @param charCount 字符数
    /// @param text 文本内容（UTF-8）
    TextRun(Paragraph* para, RunPr* runPr,
            std::uint32_t startCharPos, std::uint32_t charCount,
            const std::string& text);

    ~TextRun() override = default;

    // -----------------------------------------------------------------------
    // BaseRun 纯虚方法实现
    // -----------------------------------------------------------------------

    void Layout(IFontManager* fontMgr) override;

    float GetAscent() const override;
    float GetDescent() const override;

    std::pair<std::uint32_t, const float*> GetAdvances() const override;

    RunType GetType() const override { return RunType::kText; }

    const FontInfo* GetFontInfo() const override;

    // -----------------------------------------------------------------------
    // TextRun 特有方法
    // -----------------------------------------------------------------------

    /// 文本内容
    const std::string& GetText() const { return text_; }

    /// 设置度量数据（由 Layout 填充或外部注入）
    void SetRunMetrics(std::unique_ptr<RunMetrics>& metrics);

    /// 获取度量数据
    const RunMetrics* GetRunMetrics() const { return runMetrics_.get(); }

private:
    std::string text_;                          // 文本内容（UTF-8）
    std::unique_ptr<RunMetrics> runMetrics_;    // 度量数据（Layout 后填充）
};

} // namespace textlayout
} // namespace office
} // namespace zq
