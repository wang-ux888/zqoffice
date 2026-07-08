// =============================================================================
// src/textlayout/run_range.h
//
// RunRange 结构（Run 范围）
//
//   表示一个 Run 在文本行内的连续字符范围，是 LineRange 中"词"的基本单位。
//   由 TextLine::AddWordRange 创建并添加到 LineRange。
//
//   字段：
//     - run        : 所属 BaseRun 指针（非拥有）
//     - startChar  : 起始字符位置（段落内全局位置）
//     - charCount  : 字符数
//     - width      : 范围宽度（px）
//     - advances   : 每字符推进宽度数组（charCount 个元素）
// =============================================================================
#pragma once

#include <cstdint>
#include <vector>

namespace zq {
namespace office {
namespace textlayout {

class BaseRun;

/// Run 范围
struct RunRange {
    BaseRun* run = nullptr;             // 所属 Run（非拥有）
    std::uint32_t startChar = 0;        // 起始字符位置（段落内）
    std::uint32_t charCount = 0;        // 字符数
    float width = 0.0f;                 // 范围宽度（px）
    std::vector<float> advances;        // 每字符推进宽度（px）

    RunRange() = default;

    RunRange(BaseRun* r, std::uint32_t s, std::uint32_t c,
             float w, const float* adv)
        : run(r), startChar(s), charCount(c), width(w) {
        if (adv && c > 0) {
            advances.assign(adv, adv + c);
        }
    }

    /// 结束字符位置（不含）
    std::uint32_t GetEndChar() const { return startChar + charCount; }
};

} // namespace textlayout
} // namespace office
} // namespace zq
