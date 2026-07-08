// =============================================================================
// src/textlayout/text_line.cpp
//
// TextLine 类实现
// =============================================================================
#include "text_line.h"

#include <algorithm>
#include <cstdio>

#include "base_run.h"
#include "layout_measurer.h"
#include "layout_position.h"
#include "run_range.h"

namespace zq {
namespace office {
namespace textlayout {

// 前向声明（避免完整包含 paragraph.h / layout_page.h 形成循环）
class Paragraph;
class LayoutPage;

TextLine::TextLine(LayoutMeasurer* measurer, Paragraph* paragraph,
                   LayoutPage* page, LayoutPosition& startPos)
    : measurer_(measurer)
    , paragraph_(paragraph)
    , page_(page) {
    // 从 startPos 拷贝起始位置
    startCharIdx_ = startPos.GetCharIdx();
    startRunIdx_ = startPos.GetRunIdx();
}

TextLine::~TextLine() = default;

void TextLine::Initialize(float left, float right, float top) {
    lineLeft_ = left;
    lineRight_ = right;
    lineTop_ = top;
    lineHeight_ = 0.0f;
    maxAscent_ = 0.0f;
    maxDescent_ = 0.0f;
}

void TextLine::ClearForRelayout() {
    ranges_.clear();
    empty_ = true;
    layouted_ = false;
    charCount_ = 0;
    maxAscent_ = 0.0f;
    maxDescent_ = 0.0f;
    lineHeight_ = 0.0f;
}

void TextLine::AddBreakableRuns(LayoutPosition& pos, std::uint32_t count) {
    // 累加行内字符数
    charCount_ += count;
    // 推进 LayoutPosition（NextChar 内部更新 charIdx）
    pos.NextChar(count);
}

LineRange* TextLine::GetAvailableRange(float width, bool fromStart) {
    // 查找第一个能容纳 width 的范围
    // 若无现有范围，则创建一个覆盖整行的新范围
    if (ranges_.empty()) {
        auto range = std::make_unique<LineRange>(lineLeft_, lineRight_);
        ranges_.push_back(std::move(range));
    }

    // fromStart=true 时从行首开始查找（默认行为）
    // fromStart=false 时从最后一个范围开始查找（用于浮动对象绕行场景）
    if (fromStart) {
        for (auto& range : ranges_) {
            if (range->AvailableForWidth(width)) {
                return range.get();
            }
        }
    } else {
        for (auto it = ranges_.rbegin(); it != ranges_.rend(); ++it) {
            if ((*it)->AvailableForWidth(width)) {
                return it->get();
            }
        }
    }

    // 无可用范围时返回 nullptr（调用方需换行）
    return nullptr;
}

void TextLine::AddWordRange(BaseRun* run, std::uint32_t startChar,
                             std::uint32_t charCount, float width,
                             const float* advances) {
    // 添加到当前范围（若无范围则创建）
    if (ranges_.empty()) {
        auto range = std::make_unique<LineRange>(lineLeft_, lineRight_);
        ranges_.push_back(std::move(range));
    }

    // 取最后一个范围添加词
    ranges_.back()->AddWordRange(run, startChar, charCount, width, advances);

    // 更新行内最大 ascent/descent
    if (run) {
        maxAscent_ = std::max(maxAscent_, run->GetAscent());
        maxDescent_ = std::max(maxDescent_, run->GetDescent());
    }

    // 更新行高（ascent + descent）
    lineHeight_ = std::max(lineHeight_, maxAscent_ + maxDescent_);

    empty_ = false;
}

bool TextLine::IsEqualRangeList(
    const std::vector<std::unique_ptr<LineRange>>& other) const {
    if (ranges_.size() != other.size()) {
        return false;
    }
    for (size_t i = 0; i < ranges_.size(); ++i) {
        if (std::abs(ranges_[i]->GetXMin() - other[i]->GetXMin()) > 0.01f ||
            std::abs(ranges_[i]->GetXMax() - other[i]->GetXMax()) > 0.01f) {
            return false;
        }
    }
    return true;
}

std::uint32_t TextLine::GetCharCount() const {
    return charCount_;
}

std::uint32_t TextLine::GetStartCharPos() const {
    return startCharIdx_;
}

std::uint32_t TextLine::GetEndCharPos() const {
    return startCharIdx_ + charCount_;
}

LineRange& TextLine::GetLineRange(std::uint32_t idx) const {
    if (idx >= ranges_.size()) {
        idx = static_cast<std::uint32_t>(ranges_.size()) - 1;
    }
    return *ranges_[idx];
}

LayoutPosition TextLine::GetStartLayoutPosition() const {
    return LayoutPosition(startCharIdx_, startRunIdx_);
}

std::uint32_t TextLine::GetCharPosByCoordinateX(float x) const {
    // 遍历所有范围，找到包含 x 的范围
    std::uint32_t offset = 0;
    for (const auto& range : ranges_) {
        if (x < range->GetXMax() || &range == &ranges_.back()) {
            // 在此范围内查找
            for (std::uint32_t w = 0; w < range->GetWordCount(); ++w) {
                const RunRange* word = range->GetWord(w);
                if (!word) break;
                float wordLeft = range->GetWordLeft(w);
                float wordRight = range->GetWordRight(w);
                if (x < wordRight) {
                    // 在此词内查找字符
                    if (word->advances.empty()) {
                        return offset;
                    }
                    float cur = wordLeft;
                    for (std::uint32_t c = 0; c < word->advances.size(); ++c) {
                        if (x < cur + word->advances[c] / 2.0f) {
                            return offset + c;
                        }
                        cur += word->advances[c];
                    }
                    return offset + static_cast<std::uint32_t>(word->advances.size());
                }
                offset += word->charCount;
            }
            return offset;
        }
    }
    return charCount_;
}

std::pair<float, float> TextLine::GetCharXRange(std::uint32_t charPos) const {
    // 遍历范围与词，找到 charPos 对应的 X 范围
    std::uint32_t consumed = 0;
    for (const auto& range : ranges_) {
        for (std::uint32_t w = 0; w < range->GetWordCount(); ++w) {
            const RunRange* word = range->GetWord(w);
            if (!word) break;
            if (charPos < consumed + word->charCount) {
                // 在此词内
                std::uint32_t localChar = charPos - consumed;
                float left = range->GetWordLeft(w);
                for (std::uint32_t c = 0; c < localChar && c < word->advances.size(); ++c) {
                    left += word->advances[c];
                }
                float right = left;
                if (localChar < word->advances.size()) {
                    right += word->advances[localChar];
                }
                return { left, right };
            }
            consumed += word->charCount;
        }
    }
    return { lineLeft_, lineLeft_ };
}

std::pair<float, float> TextLine::GetXRangeByCharRange(
    std::uint32_t startChar, std::uint32_t endChar) const {
    auto startRange = GetCharXRange(startChar);
    auto endRange = GetCharXRange(endChar > 0 ? endChar - 1 : 0);
    return { startRange.first, endRange.second };
}

std::pair<std::uint32_t, std::uint32_t> TextLine::GetWordRangeByCoordinateX(float x) const {
    // 遍历范围，找到包含 x 的词
    for (const auto& range : ranges_) {
        for (std::uint32_t w = 0; w < range->GetWordCount(); ++w) {
            const RunRange* word = range->GetWord(w);
            if (!word) break;
            float wordLeft = range->GetWordLeft(w);
            float wordRight = range->GetWordRight(w);
            if (x >= wordLeft && x <= wordRight) {
                return { word->startChar, word->GetEndChar() };
            }
        }
    }
    return { 0, 0 };
}

void TextLine::UpdateLineTop(float newTop) {
    lineTop_ = newTop;
}

void TextLine::DumpLineInfo() const {
    std::printf("TextLine: top=%.2f bottom=%.2f left=%.2f right=%.2f "
                "height=%.2f ascent=%.2f descent=%.2f ranges=%zu chars=%u\n",
                lineTop_, GetLineBottom(), lineLeft_, lineRight_,
                lineHeight_, maxAscent_, maxDescent_,
                ranges_.size(), charCount_);
}

} // namespace textlayout
} // namespace office
} // namespace zq
