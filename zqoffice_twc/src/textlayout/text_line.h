// =============================================================================
// src/textlayout/text_line.h
//
// TextLine 类（文本行）
//
//   文本行是排版流水线中的核心单元，包含一组 LineRange（行内范围）。
//   每个 TextLine 持有：
//     - 起始 LayoutPosition（字符位置 + Run 索引）
//     - 行几何（top/bottom/left/right/height）
//     - 最大 ascent/descent（决定行高）
//     - LineRange 列表（按从左到右顺序）
//
//   生命周期：
//     1. 构造（绑定 measurer/paragraph/page/startPos）
//     2. Initialize(left, right, top) - 设置初始行几何
//     3. AddBreakableRuns(pos, count) - 添加可断 Run
//     4. GetAvailableRange(width, ...) - 取可用范围
//     5. AddWordRange(...) - 添加词到当前范围
//     6. SetLayouted() - 标记布局完成
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "textlayout/line_range.h"

namespace zq {
namespace office {
namespace textlayout {

class LayoutMeasurer;
class Paragraph;
class LayoutPage;
class LayoutPosition;
class BaseRun;

/// 文本行
class TextLine {
public:
    /// 构造
    /// @param measurer 布局测量器
    /// @param paragraph 所属段落
    /// @param page 所属页面
    /// @param startPos 起始布局位置
    TextLine(LayoutMeasurer* measurer, Paragraph* paragraph,
             LayoutPage* page, LayoutPosition& startPos);

    virtual ~TextLine();

    // -----------------------------------------------------------------------
    // 初始化与重置
    // -----------------------------------------------------------------------

    /// 初始化行几何
    /// @param left 行左边界
    /// @param right 行右边界
    /// @param top 行顶部
    void Initialize(float left, float right, float top);

    /// 重置布局（保留几何，清空范围列表与状态）
    void ClearForRelayout();

    /// 标记布局完成
    void SetLayouted() { layouted_ = true; }

    /// 标记非空（行内至少有一个词）
    void SetNoEmpty() { empty_ = false; }

    // -----------------------------------------------------------------------
    // Run / 词管理
    // -----------------------------------------------------------------------

    /// 添加可断 Run（从 pos 开始，count 个字符）
    /// @param pos 布局位置（in/out，返回下一个位置）
    /// @param count 字符数
    void AddBreakableRuns(LayoutPosition& pos, std::uint32_t count);

    /// 取可用范围
    /// @param width 需要的宽度
    /// @param fromStart 是否从行首开始查找
    /// @return 可用 LineRange 指针，无可用时返回 nullptr
    LineRange* GetAvailableRange(float width, bool fromStart);

    /// 添加词范围到当前行
    /// @param run 所属 Run
    /// @param startChar 起始字符位置
    /// @param charCount 字符数
    /// @param width 词宽度
    /// @param advances 每字符推进宽度
    void AddWordRange(BaseRun* run, std::uint32_t startChar,
                      std::uint32_t charCount, float width,
                      const float* advances);

    /// 范围列表是否相等（用于重布局优化）
    bool IsEqualRangeList(const std::vector<std::unique_ptr<LineRange>>& other) const;

    /// 设置范围列表（外部注入）
    void SetRangeLst(std::vector<std::unique_ptr<LineRange>> ranges) {
        ranges_ = std::move(ranges);
    }

    // -----------------------------------------------------------------------
    // 字符位置 / 计数
    // -----------------------------------------------------------------------

    /// 行内字符数
    std::uint32_t GetCharCount() const;

    /// 起始字符位置（段落内）
    std::uint32_t GetStartCharPos() const;

    /// 结束字符位置（不含）
    std::uint32_t GetEndCharPos() const;

    // -----------------------------------------------------------------------
    // 范围访问
    // -----------------------------------------------------------------------

    /// 范围数
    std::uint32_t GetRangeCount() const {
        return static_cast<std::uint32_t>(ranges_.size());
    }

    /// 取第 idx 个范围
    LineRange& GetLineRange(std::uint32_t idx) const;

    // -----------------------------------------------------------------------
    // 行几何
    // -----------------------------------------------------------------------

    float GetLineTop() const { return lineTop_; }
    float GetLineBottom() const { return lineTop_ + lineHeight_; }
    float GetLineLeft() const { return lineLeft_; }
    float GetLineRight() const { return lineRight_; }
    float GetLineHeight() const { return lineHeight_; }

    /// 最大 ascent（行内所有 Run 的最大 ascent）
    float GetMaxAscent() const { return maxAscent_; }

    /// 最大 descent（行内所有 Run 的最大 descent）
    float GetMaxDescent() const { return maxDescent_; }

    // -----------------------------------------------------------------------
    // 上下文访问
    // -----------------------------------------------------------------------

    /// 所属段落
    Paragraph* GetParagraph() const { return paragraph_; }

    /// 起始布局位置
    LayoutPosition GetStartLayoutPosition() const;

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    /// 是否空行（无词）
    bool IsEmpty() const { return empty_; }

    /// 是否段落首行
    bool IsFirstLineOfParagraph() const { return isFirstLineOfParagraph_; }

    /// 是否段落末行
    bool IsLastLineOfParagraph() const { return isLastLineOfParagraph_; }

    /// 设置首行标识
    void SetFirstLineOfParagraph(bool v) { isFirstLineOfParagraph_ = v; }

    /// 设置末行标识
    void SetLastLineOfParagraph(bool v) { isLastLineOfParagraph_ = v; }

    // -----------------------------------------------------------------------
    // 坐标查询
    // -----------------------------------------------------------------------

    /// X 坐标 → 字符位置（行内）
    /// @param x X 坐标
    /// @return 字符位置（行内偏移）
    std::uint32_t GetCharPosByCoordinateX(float x) const;

    /// 字符位置 → X 范围（行内）
    /// @param charPos 字符位置（行内偏移）
    /// @return {left, right}
    std::pair<float, float> GetCharXRange(std::uint32_t charPos) const;

    /// 整行 X 范围
    std::pair<float, float> GetXRangeForLine() const {
        return { lineLeft_, lineRight_ };
    }

    /// 字符范围 → X 范围
    /// @param startChar 起始字符位置（行内偏移）
    /// @param endChar 结束字符位置（行内偏移，不含）
    std::pair<float, float> GetXRangeByCharRange(std::uint32_t startChar,
                                                  std::uint32_t endChar) const;

    /// X 坐标 → 词范围
    /// @param x X 坐标
    /// @return {startChar, endChar}（行内偏移）
    std::pair<std::uint32_t, std::uint32_t> GetWordRangeByCoordinateX(float x) const;

    // -----------------------------------------------------------------------
    // 更新
    // -----------------------------------------------------------------------

    /// 更新行顶部（用于段后间距调整）
    /// @param newTop 新的行顶部
    void UpdateLineTop(float newTop);

    // -----------------------------------------------------------------------
    // 调试
    // -----------------------------------------------------------------------

    /// 打印行信息（调试用）
    void DumpLineInfo() const;

private:
    // 上下文（非拥有）
    LayoutMeasurer* measurer_;
    Paragraph* paragraph_;
    LayoutPage* page_;

    // 起始布局位置（拥有，因为 LayoutPosition 是值类型）
    // 实际持有 charIdx/runIdx 两个值
    std::uint32_t startCharIdx_ = 0;
    std::uint32_t startRunIdx_ = 0;

    // 行几何
    float lineLeft_ = 0.0f;
    float lineRight_ = 0.0f;
    float lineTop_ = 0.0f;
    float lineHeight_ = 0.0f;
    float maxAscent_ = 0.0f;
    float maxDescent_ = 0.0f;

    // 范围列表
    std::vector<std::unique_ptr<LineRange>> ranges_;

    // 状态
    bool empty_ = true;
    bool layouted_ = false;
    bool isFirstLineOfParagraph_ = false;
    bool isLastLineOfParagraph_ = false;

    // 行内字符数（缓存）
    std::uint32_t charCount_ = 0;
};

} // namespace textlayout
} // namespace office
} // namespace zq
