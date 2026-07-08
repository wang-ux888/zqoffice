// =============================================================================
// src/drawing/text_body.cpp
//
// TextBody 类实现
// =============================================================================
#include "text_body.h"

namespace zq {
namespace office {
namespace drawing {

TextBody::TextBody() = default;
TextBody::~TextBody() = default;

void TextBody::clear() {
    verticalAlign_ = TextAnchoringType::kTop;
    wrapText_ = TextWrapType::kSquare;
    vertClip_ = TextVertOverflowType::kOverflow;
    textFlow_ = TextVerticalType::kHorizontal;
    horzClip_ = TextHorzOverflowType::kOverflow;
    fitType_ = TextFitType::kNoAutofit;
    hasInsets_ = false;
    lIns_ = 91440;
    tIns_ = 45720;
    rIns_ = 91440;
    bIns_ = 45720;
    paragraphs_.clear();
}

} // namespace drawing
} // namespace office
} // namespace zq
