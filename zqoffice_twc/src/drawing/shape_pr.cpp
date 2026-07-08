// =============================================================================
// src/drawing/shape_pr.cpp
// =============================================================================
#include "shape_pr.h"

namespace zq {
namespace office {
namespace drawing {

ShapePr::ShapePr() = default;
ShapePr::~ShapePr() = default;

void ShapePr::clear() {
    x_ = y_ = w_ = h_ = 0;
    rotation_ = 0.0;
    flipH_ = flipV_ = false;
    shapeType_ = PrstShapeType::kUnknown;
}

} // namespace drawing
} // namespace office
} // namespace zq
