// =============================================================================
// src/drawing/tt_shape_geom.h
//
// TTShapeGeom 类：形状几何管理器
//
//   管理形状的几何属性，桥接 TextBody、ThemeNode 和 XYScale
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>

#include "drawing/enums/prst_shape_type.h"

namespace zq {
namespace office {
namespace drawing {

class TextBody;
class Theme;

/// 形状几何管理器
///
class TTShapeGeom {
public:
    TTShapeGeom();
    ~TTShapeGeom();

    // -----------------------------------------------------------------------
    // 设置依赖
    // -----------------------------------------------------------------------

    /// 设置文本体
    void SetTextBody(std::shared_ptr<TextBody> tb) { textBody_ = tb; }
    std::shared_ptr<TextBody> textBody() const { return textBody_; }

    /// 设置主题节点
    void SetThemeNode(std::shared_ptr<Theme> theme) { theme_ = theme; }
    std::shared_ptr<Theme> themeNode() const { return theme_; }

    /// 设置 X/Y 缩放
    void SetXYScale(double sx, double sy) { sx_ = sx; sy_ = sy; }
    double scaleX() const { return sx_; }
    double scaleY() const { return sy_; }

    // -----------------------------------------------------------------------
    // 形状类型
    // -----------------------------------------------------------------------

    void setShapeType(PrstShapeType t) { shapeType_ = t; }
    PrstShapeType shapeType() const { return shapeType_; }

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------

    bool isValid() const { return shapeType_ != PrstShapeType::kUnknown; }

    void clear();

private:
    std::shared_ptr<TextBody> textBody_;
    std::shared_ptr<Theme> theme_;
    double sx_ = 1.0;
    double sy_ = 1.0;
    PrstShapeType shapeType_ = PrstShapeType::kUnknown;
};

} // namespace drawing
} // namespace office
} // namespace zq
