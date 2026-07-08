// =============================================================================
// src/textlayout/layout_page.cpp
//
// LayoutPage 类实现
// =============================================================================
#include "layout_page.h"

#include <algorithm>

#include "layout_position.h"
#include "text_line.h"

namespace zq {
namespace office {
namespace textlayout {

class Paragraph;

LayoutPage::LayoutPage(float width, float height)
    : pageWidth_(width), pageHeight_(height) {}

LayoutPage::LayoutPage(float left, float top, float width, float height)
    : pageLeft_(left), pageTop_(top), pageWidth_(width), pageHeight_(height) {}

LayoutPage::~LayoutPage() = default;

void LayoutPage::AddLine(std::unique_ptr<TextLine> line) {
    if (line) {
        layoutedHeight_ = std::max(layoutedHeight_,
                                    line->GetLineBottom() - pageTop_);
        lines_.push_back(std::move(line));
    }
}

void LayoutPage::RemoveLastLine() {
    if (!lines_.empty()) {
        lines_.pop_back();
        // 重新计算 layoutedHeight_
        layoutedHeight_ = 0.0f;
        for (const auto& line : lines_) {
            layoutedHeight_ = std::max(layoutedHeight_,
                                        line->GetLineBottom() - pageTop_);
        }
    }
}

TextLine* LayoutPage::GetLastLine() const {
    if (lines_.empty()) return nullptr;
    return lines_.back().get();
}

TextLine* LayoutPage::GetLine(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(lines_.size())) {
        return nullptr;
    }
    return lines_[idx].get();
}

TextLine* LayoutPage::GetLineByCoordinateY(float y) const {
    for (const auto& line : lines_) {
        if (y >= line->GetLineTop() && y < line->GetLineBottom()) {
            return line.get();
        }
    }
    // 若超出最后一行底部，返回最后一行
    if (!lines_.empty() && y >= lines_.back()->GetLineBottom()) {
        return lines_.back().get();
    }
    return nullptr;
}

std::uint32_t LayoutPage::GetLineIdxByCharPos(std::uint32_t charPos) const {
    for (std::uint32_t i = 0; i < lines_.size(); ++i) {
        const auto& line = lines_[i];
        if (charPos >= line->GetStartCharPos() &&
            charPos < line->GetEndCharPos()) {
            return i;
        }
    }
    // 未找到时返回最后一行（容错）
    if (!lines_.empty()) {
        return static_cast<std::uint32_t>(lines_.size()) - 1;
    }
    return 0;
}

void LayoutPage::FinishLineLayout() {
    if (!lines_.empty()) {
        const auto& lastLine = lines_.back();
        layoutedHeight_ = std::max(layoutedHeight_,
                                    lastLine->GetLineBottom() - pageTop_);
    }
}

void LayoutPage::FinishParagraphLayout() {
    layoutedHeight_ += spacingAfter_;
}

bool LayoutPage::CheckAtLeastOneLine() {
    if (lines_.empty()) {
        // 创建一个空行占位
        auto pos = LayoutPosition(0, 0);
        auto line = std::make_unique<TextLine>(nullptr, nullptr, this, pos);
        line->Initialize(pageLeft_, pageLeft_ + pageWidth_, pageTop_);
        lines_.push_back(std::move(line));
        return false;
    }
    return true;
}

std::vector<std::unique_ptr<LineRange>> LayoutPage::GetRangeList(
    float& outY, float lineTop, float lineHeight, float indent) {
    std::vector<std::unique_ptr<LineRange>> ranges;
    // 默认实现：整行可用范围（不考虑浮动对象）
    // lineHeight 用于校验行是否超出页面底部（超出则返回空列表）
    if (lineTop + lineHeight > pageTop_ + pageHeight_ && !lastLineCanOverflow_) {
        outY = lineTop;
        return ranges;
    }
    float left = pageLeft_ + indent;
    float right = pageLeft_ + pageWidth_;
    ranges.push_back(std::make_unique<LineRange>(left, right));
    outY = lineTop;
    return ranges;
}

bool LayoutPage::ProcessFloatObject(Paragraph* /*paragraph*/,
                                     LayoutPosition /*pos*/, int /*side*/,
                                     float /*width*/) {
    // 默认实现：不处理浮动对象
    // 派生类可覆盖此方法以支持图文绕排
    return false;
}

float LayoutPage::MeasureSpace(bool isCJK, float fontSize,
                                float fontAscent, float fontDescent) {
    // 空格宽度估算：
    // - CJK 空格：约等于一个字符宽 = fontSize（基于字体高度 ascent+descent）
    // - 西文空格：约等于 fontSize * 0.25
    // fontAscent/fontDescent 用于精确校准（CJK 时按字体高度比例）
    if (isCJK) {
        float fontHeight = fontAscent + fontDescent;
        if (fontHeight > 0.0f) {
            return fontHeight;
        }
        return fontSize;
    }
    return fontSize * 0.25f;
}

float LayoutPage::GetCharPosByXCoordinateInLine(std::uint32_t lineIdx,
                                                  std::uint32_t startCharPos,
                                                  float x) const {
    if (lineIdx >= lines_.size()) {
        return static_cast<float>(startCharPos);
    }
    const auto& line = lines_[lineIdx];
    std::uint32_t localPos = line->GetCharPosByCoordinateX(x);
    return static_cast<float>(startCharPos + localPos);
}

std::tuple<float, float, float, float> LayoutPage::GetRectForCharInLine(
    std::uint32_t lineIdx, std::uint32_t charPos) const {
    if (lineIdx >= lines_.size()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    const auto& line = lines_[lineIdx];
    auto xRange = line->GetCharXRange(charPos);
    return { xRange.first, line->GetLineTop(),
             xRange.second, line->GetLineBottom() };
}

std::tuple<float, float, float, float> LayoutPage::GetRectForLine(
    std::uint32_t lineIdx) const {
    if (lineIdx >= lines_.size()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    const auto& line = lines_[lineIdx];
    auto xRange = line->GetXRangeForLine();
    return { xRange.first, line->GetLineTop(),
             xRange.second, line->GetLineBottom() };
}

std::vector<std::tuple<float, float, float, float>>
LayoutPage::GetRectsForTextRange(std::uint32_t startChar,
                                  std::uint32_t endChar) const {
    std::vector<std::tuple<float, float, float, float>> rects;
    if (startChar >= endChar || lines_.empty()) {
        return rects;
    }

    // 找到包含 startChar 的行
    std::uint32_t startLineIdx = GetLineIdxByCharPos(startChar);
    std::uint32_t endLineIdx = GetLineIdxByCharPos(endChar > 0 ? endChar - 1 : 0);

    for (std::uint32_t i = startLineIdx; i <= endLineIdx && i < lines_.size(); ++i) {
        const auto& line = lines_[i];
        std::uint32_t lineStart = line->GetStartCharPos();
        std::uint32_t lineEnd = line->GetEndCharPos();

        // 计算行内字符范围
        std::uint32_t localStart = (startChar > lineStart) ? (startChar - lineStart) : 0;
        std::uint32_t localEnd = (endChar < lineEnd) ? (endChar - lineStart) : (lineEnd - lineStart);

        auto xRange = line->GetXRangeByCharRange(localStart, localEnd);
        rects.push_back({ xRange.first, line->GetLineTop(),
                          xRange.second, line->GetLineBottom() });
    }
    return rects;
}

std::pair<std::uint32_t, std::uint32_t>
LayoutPage::GetWordRangeByCoordinate(float x, float y) const {
    TextLine* line = GetLineByCoordinateY(y);
    if (!line) {
        return { 0, 0 };
    }
    return line->GetWordRangeByCoordinateX(x);
}

std::tuple<float, float, float, float> LayoutPage::ToRectTuple(
    std::pair<float, float>& xRange, TextLine* line) {
    if (!line) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    return { xRange.first, line->GetLineTop(),
             xRange.second, line->GetLineBottom() };
}

} // namespace textlayout
} // namespace office
} // namespace zq
