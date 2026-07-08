// =============================================================================
// src/textlayout/layout_position.h
//
// LayoutPosition 类（布局位置）
//
//   表示布局过程中的位置坐标，由两个索引组成：
//     - charIdx : 段落内字符位置
//     - runIdx  : 段落内 Run 索引
//
//   用于 TextLine / LayoutPage 的布局流水线中传递当前位置。
//   提供全套比较运算符（用于布局回退与位置比较）。
//
//   语义：
//     - NextChar(count) : charIdx += count
//     - NextRun()       : runIdx += 1
//     - Update(other)   : 取较大值（推进到更后的位置）
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {
namespace textlayout {

/// 布局位置
class LayoutPosition {
public:
    LayoutPosition() = default;

    /// 构造
    /// @param charIdx 字符索引
    /// @param runIdx Run 索引
    LayoutPosition(std::uint32_t charIdx, std::uint32_t runIdx)
        : charIdx_(charIdx), runIdx_(runIdx) {}

    ~LayoutPosition() = default;

    LayoutPosition(const LayoutPosition&) = default;
    LayoutPosition& operator=(const LayoutPosition&) = default;
    LayoutPosition(LayoutPosition&&) noexcept = default;
    LayoutPosition& operator=(LayoutPosition&&) noexcept = default;

    // -----------------------------------------------------------------------
    // 索引访问
    // -----------------------------------------------------------------------

    /// 字符索引
    std::uint32_t GetCharIdx() const { return charIdx_; }

    /// Run 索引
    std::uint32_t GetRunIdx() const { return runIdx_; }

    // -----------------------------------------------------------------------
    // 索引设置
    // -----------------------------------------------------------------------

    /// 设置字符索引
    void SetCharIdx(std::uint32_t idx) { charIdx_ = idx; }

    /// 设置 Run 索引
    void SetRunIdx(std::uint32_t idx) { runIdx_ = idx; }

    // -----------------------------------------------------------------------
    // 推进
    // -----------------------------------------------------------------------

    /// 推进字符位置
    /// @param count 字符数
    void NextChar(std::uint32_t count) { charIdx_ += count; }

    /// 推进 Run 位置
    void NextRun() { runIdx_ += 1; }

    // -----------------------------------------------------------------------
    // 更新
    // -----------------------------------------------------------------------

    /// 更新到较大位置（用于布局回退后取较后位置）
    /// @param other 另一个位置
    void Update(const LayoutPosition& other) {
        if (other.charIdx_ > charIdx_) charIdx_ = other.charIdx_;
        if (other.runIdx_ > runIdx_) runIdx_ = other.runIdx_;
    }

    // -----------------------------------------------------------------------
    // 比较运算符（全套）
    // -----------------------------------------------------------------------

    bool operator==(const LayoutPosition& other) const {
        return charIdx_ == other.charIdx_ && runIdx_ == other.runIdx_;
    }

    bool operator!=(const LayoutPosition& other) const {
        return !(*this == other);
    }

    bool operator<(const LayoutPosition& other) const {
        if (charIdx_ != other.charIdx_) return charIdx_ < other.charIdx_;
        return runIdx_ < other.runIdx_;
    }

    bool operator>(const LayoutPosition& other) const {
        return other < *this;
    }

    bool operator<=(const LayoutPosition& other) const {
        return !(other < *this);
    }

    bool operator>=(const LayoutPosition& other) const {
        return !(*this < other);
    }

private:
    std::uint32_t charIdx_ = 0;     // 字符索引
    std::uint32_t runIdx_ = 0;      // Run 索引
};

} // namespace textlayout
} // namespace office
} // namespace zq
