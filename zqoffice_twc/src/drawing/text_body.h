// =============================================================================
// src/drawing/text_body.h
//
// TextBody 类：OOXML 文本体模型
//
//   对应 OOXML ECMA-376 第 20.1.2.2 节 a:txBody 元素，包含：
//     - a:bodyPr : 文本体属性（anchor/wrap/vertOverflow/horzOverflow/vert 等）
//     - a:p      : 段落列表（每个段落包含 a:r 文本运行）
//
//   API 风格：
//     verticalAlign / wrapText / vertClip / textFlow / horzClip
//     子元素管理（paragraph / run）
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "drawing/enums/text_enums.h"

namespace zq {
namespace office {
namespace drawing {

/// 文本运行（a:r）
struct TextRun {
    std::string text;           // a:t 文本内容
    std::string fontColor;      // a:rPr/a:solidFill/a:srgbClr val (RRGGBB)
    std::string fontName;       // a:rPr typeface
    double fontSize = -1.0;     // a:rPr sz (pt, -1 = 未设置)
    bool bold = false;          // a:rPr b
    bool italic = false;        // a:rPr i
    bool valid = false;
};

/// 文本段落（a:p）
struct TextParagraph {
    std::vector<TextRun> runs;              // a:r 文本运行列表
    TextAlignType align = TextAlignType::kLeft;  // a:pPr algn
    double indent = 0.0;                    // a:pPr indent (EMU)
    double marL = 0.0;                      // a:pPr marL (EMU)
    bool valid = false;
};

/// 文本体（a:txBody）
///
class TextBody {
public:
    TextBody();
    ~TextBody();

    // -----------------------------------------------------------------------
    // bodyPr 属性（a:bodyPr）
    // -----------------------------------------------------------------------

    /// 垂直锚定（a:bodyPr anchor）
    void setVerticalAlign(TextAnchoringType t) { verticalAlign_ = t; }
    TextAnchoringType verticalAlign() const { return verticalAlign_; }

    /// 换行（a:bodyPr wrap）
    void setWrapText(TextWrapType t) { wrapText_ = t; }
    TextWrapType wrapText() const { return wrapText_; }

    /// 垂直裁剪/溢出（a:bodyPr vertOverflow）
    void setVertClip(TextVertOverflowType t) { vertClip_ = t; }
    TextVertOverflowType vertClip() const { return vertClip_; }

    /// 文本流向（a:bodyPr vert）
    void setTextFlow(TextVerticalType t) { textFlow_ = t; }
    TextVerticalType textFlow() const { return textFlow_; }

    /// 水平溢出（a:bodyPr horzOverflow）
    void setHorzClip(TextHorzOverflowType t) { horzClip_ = t; }
    TextHorzOverflowType horzClip() const { return horzClip_; }

    /// 自动适应（a:bodyPr spAutoFit / normAutofit / noAutofit）
    void setFitType(TextFitType t) { fitType_ = t; }
    TextFitType fitType() const { return fitType_; }

    /// 文本边距（a:bodyPr lIns/rIns/tIns/bIns，单位 EMU）
    void setInsets(std::int64_t l, std::int64_t t, std::int64_t r, std::int64_t b) {
        lIns_ = l; tIns_ = t; rIns_ = r; bIns_ = b;
        hasInsets_ = true;
    }
    bool hasInsets() const { return hasInsets_; }
    std::int64_t lIns() const { return lIns_; }
    std::int64_t tIns() const { return tIns_; }
    std::int64_t rIns() const { return rIns_; }
    std::int64_t bIns() const { return bIns_; }

    // -----------------------------------------------------------------------
    // 段落子元素管理（a:p）
    // -----------------------------------------------------------------------

    /// 添加段落
    void addParagraph(const TextParagraph& p) { paragraphs_.push_back(p); }

    /// 获取所有段落
    const std::vector<TextParagraph>& paragraphs() const { return paragraphs_; }
    std::vector<TextParagraph>& paragraphs() { return paragraphs_; }

    /// 段落数量
    std::size_t paragraphCount() const { return paragraphs_.size(); }

    /// 获取指定段落
    const TextParagraph& paragraph(std::size_t i) const { return paragraphs_[i]; }
    TextParagraph& paragraph(std::size_t i) { return paragraphs_[i]; }

    /// 清空段落
    void clearParagraphs() { paragraphs_.clear(); }

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------

    /// 是否有效（至少包含 1 个段落）
    bool isValid() const { return !paragraphs_.empty(); }

    /// 清空所有数据
    void clear();

private:
    // bodyPr 属性
    TextAnchoringType verticalAlign_ = TextAnchoringType::kTop;
    TextWrapType wrapText_ = TextWrapType::kSquare;
    TextVertOverflowType vertClip_ = TextVertOverflowType::kOverflow;
    TextVerticalType textFlow_ = TextVerticalType::kHorizontal;
    TextHorzOverflowType horzClip_ = TextHorzOverflowType::kOverflow;
    TextFitType fitType_ = TextFitType::kNoAutofit;

    bool hasInsets_ = false;
    std::int64_t lIns_ = 91440;   // 0.1 inch 默认值
    std::int64_t tIns_ = 45720;   // 0.05 inch
    std::int64_t rIns_ = 91440;
    std::int64_t bIns_ = 45720;

    // 段落列表
    std::vector<TextParagraph> paragraphs_;
};

} // namespace drawing
} // namespace office
} // namespace zq
