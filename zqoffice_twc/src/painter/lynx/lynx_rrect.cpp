// =============================================================================
// src/painter/lynx/lynx_rrect.cpp
//
// LynxRRect 类实现
// =============================================================================
#include "painter/lynx/lynx_rrect.h"

namespace zq {
namespace office {
namespace painter {
namespace lynx {

LynxRRect::LynxRRect()
    : left_(0.0f), top_(0.0f), right_(0.0f), bottom_(0.0f) {}

LynxRRect::LynxRRect(float left, float top, float right, float bottom)
    : left_(left), top_(top), right_(right), bottom_(bottom) {}

void LynxRRect::offset(float dx, float dy) {
    left_ += dx;
    top_ += dy;
    right_ += dx;
    bottom_ += dy;
}

void LynxRRect::SetRect(float left, float top, float right, float bottom) {
    left_ = left;
    top_ = top;
    right_ = right;
    bottom_ = bottom;
}

}  // namespace lynx
}  // namespace painter
}  // namespace office
}  // namespace zq
