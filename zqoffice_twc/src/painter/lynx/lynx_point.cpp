// =============================================================================
// src/painter/lynx/lynx_point.cpp
//
// LynxPoint 类实现
// =============================================================================
#include "painter/lynx/lynx_point.h"

namespace zq {
namespace office {
namespace painter {
namespace lynx {

LynxPoint::LynxPoint() : x_(0.0f), y_(0.0f) {}

LynxPoint::LynxPoint(float x, float y) : x_(x), y_(y) {}

void LynxPoint::SetX(int x) { x_ = static_cast<float>(x); }
void LynxPoint::SetX(float x) { x_ = x; }

void LynxPoint::SetY(int y) { y_ = static_cast<float>(y); }
void LynxPoint::SetY(float y) { y_ = y; }

void LynxPoint::Set(float x, float y) {
    x_ = x;
    y_ = y;
}

void LynxPoint::Set(int x, int y) {
    x_ = static_cast<float>(x);
    y_ = static_cast<float>(y);
}

}  // namespace lynx
}  // namespace painter
}  // namespace office
}  // namespace zq
