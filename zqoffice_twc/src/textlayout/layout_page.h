// =============================================================================
// src/textlayout/layout_page.h
//
// LayoutPage 类（布局页面）
//
//   布局页面是排版流水线的顶层容器，持有一组 TextLine（按从上到下顺序）。
//
//   职责：
//     - 管理页面几何（width/height + 已布局高度 layoutedHeight_）
//     - 管理行列表（AddLine/RemoveLastLine/GetLine）
//     - 行距基础（linePitch）与段后间距（spacingAfter）
//     - 浮动对象处理（ProcessFloatObject）影响后续行的可用范围
//     - 提供 GetRangeList 取行内可用范围列表（考虑浮动对象绕行）
//     - 提供坐标查询（X→字符位置、字符→矩形、行→矩形、文本范围→矩形集）
//     - 行换行与溢出控制（wrapText/lastLineCanOverflow/fullFilled）
//
//   生命周期：
//     1. 构造（页面尺寸）
//     2. AddLine(unique_ptr<TextLine>) - 添加已布局的行
//     3. FinishLineLayout / FinishParagraphLayout - 结束行/段落布局
//     4. GetLine/GetRectForLine/GetRectForCharInLine - 查询
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "textlayout/line_range.h"

namespace zq {
namespace office {
namespace textlayout {

class TextLine;
class Paragraph;
class LayoutPosition;

/// 布局页面
class LayoutPage {
public:
    /// 构造（页面尺寸）
    /// @param width 页面宽度
    /// @param height 页面高度
    LayoutPage(float width, float height);

    /// 构造（带边距）
    /// @param left 左边距
    /// @param top 顶边距
    /// @param width 内容区宽度
    /// @param height 内容区高度
    LayoutPage(float left, float top, float width, float height);

    virtual ~LayoutPage();

    // -----------------------------------------------------------------------
    // 行管理
    // -----------------------------------------------------------------------

    /// 添加已布局的行
    /// @param line 行（转移所有权）
    void AddLine(std::unique_ptr<TextLine> line);

    /// 删除末行
    void RemoveLastLine();

    /// 取末行
    TextLine* GetLastLine() const;

    /// 取第 idx 行
    TextLine* GetLine(int idx) const;

    /// 按 Y 坐标取行
    TextLine* GetLineByCoordinateY(float y) const;

    /// 行数
    std::uint32_t GetLineCount() const {
        return static_cast<std::uint32_t>(lines_.size());
    }

    /// 按字符位置取行号
    std::uint32_t GetLineIdxByCharPos(std::uint32_t charPos) const;

    // -----------------------------------------------------------------------
    // 结束布局
    // -----------------------------------------------------------------------

    /// 结束行布局（更新 layoutedHeight_）
    void FinishLineLayout();

    /// 结束段落布局（应用段后间距）
    void FinishParagraphLayout();

    /// 至少一行（页面不允许空，至少保留一个空行）
    virtual bool CheckAtLeastOneLine();

    // -----------------------------------------------------------------------
    // 范围列表
    // -----------------------------------------------------------------------

    /// 取行内可用范围列表（考虑浮动对象绕行）
    /// @param outY 输出：行顶部 Y 坐标
    /// @param lineTop 行顶部
    /// @param lineHeight 行高
    /// @param indent 缩进
    /// @return 范围列表
    virtual std::vector<std::unique_ptr<LineRange>> GetRangeList(
        float& outY, float lineTop, float lineHeight, float indent);

    /// 处理浮动对象（影响后续行的可用范围）
    /// @param paragraph 段落
    /// @param pos 布局位置
    /// @param side 边（0=左，1=右，2=两边）
    /// @param width 浮动对象宽度
    /// @return 是否成功
    virtual bool ProcessFloatObject(Paragraph* paragraph,
                                     LayoutPosition pos, int side,
                                     float width);

    // -----------------------------------------------------------------------
    // 页面几何
    // -----------------------------------------------------------------------

    float GetPageWidth() const { return pageWidth_; }
    float GetPageHeight() const { return pageHeight_; }
    float GetLayoutedHeight() const { return layoutedHeight_; }

    // -----------------------------------------------------------------------
    // 行距 / 段后间距
    // -----------------------------------------------------------------------

    float GetLinePitch() const { return linePitch_; }
    void SetLinePitch(float v) { linePitch_ = v; }

    float GetSpacingAfter() const { return spacingAfter_; }
    void SetSpacingAfter(float v) { spacingAfter_ = v; }

    // -----------------------------------------------------------------------
    // 空格测量
    // -----------------------------------------------------------------------

    /// 测量空格宽
    /// @param isCJK 是否 CJK 字符的空格
    /// @param fontSize 字号
    /// @param fontAscent 字体 ascent
    /// @param fontDescent 字体 descent
    /// @return 空格宽度（px）
    float MeasureSpace(bool isCJK, float fontSize,
                       float fontAscent, float fontDescent);

    // -----------------------------------------------------------------------
    // 状态开关
    // -----------------------------------------------------------------------

    bool IsFullFilled() const { return fullFilled_; }
    void SetFullFilled(bool v) { fullFilled_ = v; }

    bool IsLastLineCanOverflow() const { return lastLineCanOverflow_; }
    void SetLastLineCanOverflow(bool v) { lastLineCanOverflow_ = v; }

    bool IsWrapText() const { return wrapText_; }
    void EnableWrapText(bool v) { wrapText_ = v; }

    void SetSkipSpacingBeforeFirstLine(bool v) { skipSpacingBeforeFirstLine_ = v; }
    bool IsSkipSpacingBeforeFirstLine() const { return skipSpacingBeforeFirstLine_; }

    // -----------------------------------------------------------------------
    // 坐标查询
    // -----------------------------------------------------------------------

    /// X 坐标 → 字符位置（指定行内）
    /// @param lineIdx 行号
    /// @param startCharPos 行起始字符位置（段落内）
    /// @param x X 坐标
    /// @return 字符位置（段落内）
    float GetCharPosByXCoordinateInLine(std::uint32_t lineIdx,
                                         std::uint32_t startCharPos,
                                         float x) const;

    /// 字符矩形（指定行内）
    /// @param lineIdx 行号
    /// @param charPos 字符位置（行内偏移）
    /// @return {left, top, right, bottom}
    std::tuple<float, float, float, float> GetRectForCharInLine(
        std::uint32_t lineIdx, std::uint32_t charPos) const;

    /// 行矩形
    /// @param lineIdx 行号
    /// @return {left, top, right, bottom}
    std::tuple<float, float, float, float> GetRectForLine(
        std::uint32_t lineIdx) const;

    /// 文本范围矩形集
    /// @param startChar 起始字符位置（段落内）
    /// @param endChar 结束字符位置（段落内）
    /// @return 矩形列表
    std::vector<std::tuple<float, float, float, float>> GetRectsForTextRange(
        std::uint32_t startChar, std::uint32_t endChar) const;

    /// 坐标 → 词范围
    /// @param x X 坐标
    /// @param y Y 坐标
    /// @return {startChar, endChar}（段落内）
    std::pair<std::uint32_t, std::uint32_t> GetWordRangeByCoordinate(
        float x, float y) const;

    /// 矩形 tuple 转换
    std::tuple<float, float, float, float> ToRectTuple(
        std::pair<float, float>& xRange, TextLine* line);

protected:
    // 页面几何
    float pageLeft_ = 0.0f;     // 页面左边距
    float pageTop_ = 0.0f;      // 页面顶边距
    float pageWidth_ = 0.0f;    // 页面宽度
    float pageHeight_ = 0.0f;   // 页面高度
    float layoutedHeight_ = 0.0f; // 已布局高度

    // 行距 / 段后
    float linePitch_ = 0.0f;
    float spacingAfter_ = 0.0f;

    // 状态
    bool fullFilled_ = false;
    bool lastLineCanOverflow_ = false;
    bool wrapText_ = true;
    bool skipSpacingBeforeFirstLine_ = false;

    // 行列表
    std::vector<std::unique_ptr<TextLine>> lines_;
};

} // namespace textlayout
} // namespace office
} // namespace zq
