// =============================================================================
// src/drawing/tt_shape.cpp
// =============================================================================
#include "tt_shape.h"

namespace zq {
namespace office {
namespace drawing {

TTShape::TTShape() = default;
TTShape::~TTShape() = default;

void TTShape::clear() {
    x_ = y_ = w_ = h_ = 0;
    sx_ = sy_ = 1.0;
    shapeType_ = PrstShapeType::kUnknown;
    name_.clear();
    textBody_.reset();
    shapePr_.reset();
}

} // namespace drawing
} // namespace office
} // namespace zq
