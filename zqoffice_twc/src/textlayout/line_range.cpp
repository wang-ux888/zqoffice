// =============================================================================
// src/textlayout/line_range.cpp
//
// LineRange 类实现
// =============================================================================
#include "line_range.h"

#include "base_run.h"

namespace zq {
namespace office {
namespace textlayout {

LineRange::LineRange(float xMin, float xMax)
    : xMin_(xMin), xMax_(xMax) {}

void LineRange::Clear() {
    words_.clear();
    wordSpace_ = 0.0f;
}

void LineRange::AddWordRange(BaseRun* run, std::uint32_t startChar,
                              std::uint32_t charCount, float width,
                              const float* advances) {
    words_.push_back(std::make_unique<RunRange>(
        run, startChar, charCount, width, advances));
}

void LineRange::DistributeSpace(float space) {
    // 词数 > 1 时，将 space 均匀分配到词间间距
    // 词数 <= 1 时，无词间间距
    if (words_.size() > 1) {
        wordSpace_ = space / static_cast<float>(words_.size() - 1);
    } else {
        wordSpace_ = 0.0f;
    }
}

float LineRange::GetContentLeft() const {
    if (words_.empty()) {
        return xMin_;
    }
    return GetWordLeft(0);
}

float LineRange::GetContentRight() const {
    if (words_.empty()) {
        return xMin_;
    }
    return GetWordRight(static_cast<std::uint32_t>(words_.size()) - 1);
}

float LineRange::GetContentWidth() const {
    return GetContentRight() - GetContentLeft();
}

bool LineRange::AvailableForWidth(float width) const {
    // 已用宽度 = 最后一个词的 right - xMin
    // 剩余 = xMax - 已用宽度
    float used = 0.0f;
    if (!words_.empty()) {
        used = GetWordRight(static_cast<std::uint32_t>(words_.size()) - 1) - xMin_;
    }
    float remaining = xMax_ - xMin_ - used;
    return remaining >= width;
}

const RunRange* LineRange::GetWord(std::uint32_t idx) const {
    if (idx >= words_.size()) {
        return nullptr;
    }
    return words_[idx].get();
}

float LineRange::GetWordLeft(std::uint32_t idx) const {
    if (idx >= words_.size()) {
        return xMin_;
    }
    // 第 idx 个词的 left = xMin + 前面所有词宽度 + 前面词间间距
    float left = xMin_;
    for (std::uint32_t i = 0; i < idx; ++i) {
        left += words_[i]->width;
    }
    // 加上 idx 个词之前的词间间距（每两个词之间一个间距）
    if (idx > 0) {
        left += wordSpace_ * static_cast<float>(idx);
    }
    return left;
}

float LineRange::GetWordRight(std::uint32_t idx) const {
    if (idx >= words_.size()) {
        return xMin_;
    }
    return GetWordLeft(idx) + words_[idx]->width;
}

float LineRange::GetWordOffset(std::uint32_t idx) const {
    return GetWordLeft(idx) - GetContentLeft();
}

} // namespace textlayout
} // namespace office
} // namespace zq
