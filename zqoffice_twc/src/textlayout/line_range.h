// =============================================================================
// src/textlayout/line_range.h
//
// LineRange 类（行内范围）
//
//   表示文本行（TextLine）内的一个连续可放置范围 [xMin, xMax]。
//   一个 TextLine 可包含多个 LineRange（例如浮动对象占据中间区域时，
//   左右两侧各形成一个 LineRange）。
//
//   每个 LineRange 内含一组"词"（RunRange），按添加顺序排列。
//   分布对齐（DistributeSpace）时按比例分配剩余空间到词间间距。
//
//   几何约定：
//     - xMin/xMax : 范围左右边界（px）
//     - contentLeft  : 当前已放置内容左边界（= 第一个词的 left）
//     - contentRight : 当前已放置内容右边界（= 最后一个词的 right）
//     - contentWidth : contentRight - contentLeft
//     - wordSpace    : 词间间距（DistributeSpace 后的非零值，用于 justify）
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "textlayout/run_range.h"

namespace zq {
namespace office {
namespace textlayout {

class BaseRun;

/// 行内范围
class LineRange {
public:
    /// 构造
    /// @param xMin 范围左边界（px）
    /// @param xMax 范围右边界（px）
    LineRange(float xMin, float xMax);

    ~LineRange() = default;

    // -----------------------------------------------------------------------
    // 状态管理
    // -----------------------------------------------------------------------

    /// 清空所有词
    void Clear();

    // -----------------------------------------------------------------------
    // 添加词
    // -----------------------------------------------------------------------

    /// 添加词范围
    /// @param run 所属 Run
    /// @param startChar 起始字符位置（段落内）
    /// @param charCount 字符数
    /// @param width 词宽度（px）
    /// @param advances 每字符推进宽度数组（可选，可为 nullptr）
    void AddWordRange(BaseRun* run, std::uint32_t startChar,
                      std::uint32_t charCount, float width,
                      const float* advances);

    // -----------------------------------------------------------------------
    // 空间分配
    // -----------------------------------------------------------------------

    /// 分配剩余空间（用于两端对齐 / 分散对齐）
    /// 将 space 均匀分配到词间间距（wordSpace_）
    /// @param space 待分配的总空间（px）
    void DistributeSpace(float space);

    // -----------------------------------------------------------------------
    // 几何查询
    // -----------------------------------------------------------------------

    /// 内容左边界（第一个词的 left，无词时返回 xMin）
    float GetContentLeft() const;

    /// 内容右边界（最后一个词的 right，无词时返回 xMin）
    float GetContentRight() const;

    /// 内容宽度（contentRight - contentLeft）
    float GetContentWidth() const;

    /// 范围左边界
    float GetXMin() const { return xMin_; }

    /// 范围右边界
    float GetXMax() const { return xMax_; }

    /// 词间间距（DistributeSpace 后的值）
    float GetWordSpace() const { return wordSpace_; }

    /// 范围宽度（xMax - xMin）
    float GetRangeWidth() const { return xMax_ - xMin_; }

    /// 指定宽度是否可用（剩余空间 >= width）
    /// @param width 需要的宽度
    /// @return true 表示剩余空间足够
    bool AvailableForWidth(float width) const;

    // -----------------------------------------------------------------------
    // 词访问
    // -----------------------------------------------------------------------

    /// 取第 idx 个词
    /// @param idx 词索引
    /// @return 词指针，越界返回 nullptr
    const RunRange* GetWord(std::uint32_t idx) const;

    /// 词数
    std::uint32_t GetWordCount() const {
        return static_cast<std::uint32_t>(words_.size());
    }

    // -----------------------------------------------------------------------
    // 词几何
    // -----------------------------------------------------------------------

    /// 第 idx 个词的左边界
    float GetWordLeft(std::uint32_t idx) const;

    /// 第 idx 个词的右边界
    float GetWordRight(std::uint32_t idx) const;

    /// 第 idx 个词相对于内容起始的偏移
    float GetWordOffset(std::uint32_t idx) const;

    // -----------------------------------------------------------------------
    // 设置
    // -----------------------------------------------------------------------

    /// 设置范围左边界
    void SetXMin(float v) { xMin_ = v; }

    /// 设置范围右边界
    void SetXMax(float v) { xMax_ = v; }

private:
    float xMin_ = 0.0f;                             // 范围左边界
    float xMax_ = 0.0f;                             // 范围右边界
    float wordSpace_ = 0.0f;                        // 词间间距（DistributeSpace 后）
    std::vector<std::unique_ptr<RunRange>> words_;  // 词列表
};

} // namespace textlayout
} // namespace office
} // namespace zq
