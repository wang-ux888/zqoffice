// =============================================================================
// src/drawing/shape_pr.h
//
// ShapePr 类：形状属性
//
//   表示形状的几何属性（位置、尺寸、旋转等）
// =============================================================================
#pragma once

#include <cstdint>

#include "drawing/enums/prst_shape_type.h"

namespace zq {
namespace office {
namespace drawing {

/// 形状属性
///
class ShapePr {
public:
    ShapePr();
    ~ShapePr();

    // -----------------------------------------------------------------------
    // 位置与尺寸（EMU 单位）
    // -----------------------------------------------------------------------

    std::int64_t X() const { return x_; }
    std::int64_t Y() const { return y_; }
    std::int64_t W() const { return w_; }
    std::int64_t H() const { return h_; }

    void SetX(std::int64_t x) { x_ = x; }
    void SetY(std::int64_t y) { y_ = y; }
    void SetW(std::int64_t w) { w_ = w; }
    void SetH(std::int64_t h) { h_ = h; }

    // -----------------------------------------------------------------------
    // 旋转
    // -----------------------------------------------------------------------

    /// 旋转角度（度，顺时针）
    void setRotation(double rot) { rotation_ = rot; }
    double rotation() const { return rotation_; }

    /// 是否水平翻转
    void setFlipH(bool f) { flipH_ = f; }
    bool flipH() const { return flipH_; }

    /// 是否垂直翻转
    void setFlipV(bool f) { flipV_ = f; }
    bool flipV() const { return flipV_; }

    // -----------------------------------------------------------------------
    // 形状类型
    // -----------------------------------------------------------------------

    void setShapeType(PrstShapeType t) { shapeType_ = t; }
    PrstShapeType shapeType() const { return shapeType_; }

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------

    bool isValid() const { return w_ > 0 && h_ > 0; }

    void clear();

private:
    std::int64_t x_ = 0, y_ = 0, w_ = 0, h_ = 0;
    double rotation_ = 0.0;
    bool flipH_ = false;
    bool flipV_ = false;
    PrstShapeType shapeType_ = PrstShapeType::kUnknown;
};

} // namespace drawing
} // namespace office
} // namespace zq
