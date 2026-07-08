// =============================================================================
// src/drawing/tt_shape_geom.cpp
// =============================================================================
#include "tt_shape_geom.h"

namespace zq {
namespace office {
namespace drawing {

TTShapeGeom::TTShapeGeom() = default;
TTShapeGeom::~TTShapeGeom() = default;

void TTShapeGeom::clear() {
    textBody_.reset();
    theme_.reset();
    sx_ = sy_ = 1.0;
    shapeType_ = PrstShapeType::kUnknown;
}

} // namespace drawing
} // namespace office
} // namespace zq
