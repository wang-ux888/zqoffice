// =============================================================================
// src/drawing/tt_shape.h
//
// TTShape 类：转换后的形状模型
//
//   表示从 OOXML/二进制格式转换后的统一形状模型，包含：
//     - 位置与尺寸（X/Y/W/H，EMU 单位）
//     - 缩放比例（SetXYScale）
//     - 形状类型（PrstShapeType）
//     - 文本体（TextBody）
//     - 形状属性（ShapePr）
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "drawing/enums/prst_shape_type.h"
#include "drawing/text_body.h"

namespace zq {
namespace office {
namespace drawing {

class ShapePr;
class Theme;

/// 转换后的形状模型
///
class TTShape {
public:
    TTShape();
    ~TTShape();

    // -----------------------------------------------------------------------
    // 位置与尺寸（EMU 单位）
    // -----------------------------------------------------------------------

    /// X 坐标（EMU）
    std::int64_t X() const { return x_; }
    std::int64_t Y() const { return y_; }
    std::int64_t W() const { return w_; }
    std::int64_t H() const { return h_; }

    void SetX(std::int64_t x) { x_ = x; }
    void SetY(std::int64_t y) { y_ = y; }
    void SetWidth(std::int64_t w) { w_ = w; }
    void SetHeight(std::int64_t h) { h_ = h; }

    /// 同时设置位置
    void SetPosition(std::int64_t x, std::int64_t y) { x_ = x; y_ = y; }

    /// 同时设置尺寸
    void SetSize(std::int64_t w, std::int64_t h) { w_ = w; h_ = h; }

    // -----------------------------------------------------------------------
    // 缩放
    // -----------------------------------------------------------------------

    /// 设置 X/Y 缩放比例
    void SetXYScale(double sx, double sy) { sx_ = sx; sy_ = sy; }
    double scaleX() const { return sx_; }
    double scaleY() const { return sy_; }

    // -----------------------------------------------------------------------
    // 形状属性
    // -----------------------------------------------------------------------

    /// 形状类型
    void setShapeType(PrstShapeType t) { shapeType_ = t; }
    PrstShapeType shapeType() const { return shapeType_; }

    /// 形状名称
    void setName(const std::string& n) { name_ = n; }
    const std::string& name() const { return name_; }

    /// 文本体
    void setTextBody(std::shared_ptr<TextBody> tb) { textBody_ = tb; }
    std::shared_ptr<TextBody> textBody() const { return textBody_; }

    /// 形状属性
    void setShapePr(std::shared_ptr<ShapePr> sp) { shapePr_ = sp; }
    std::shared_ptr<ShapePr> shapePr() const { return shapePr_; }

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------

    bool isValid() const { return w_ > 0 && h_ > 0; }

    void clear();

private:
    std::int64_t x_ = 0, y_ = 0, w_ = 0, h_ = 0;
    double sx_ = 1.0, sy_ = 1.0;
    PrstShapeType shapeType_ = PrstShapeType::kUnknown;
    std::string name_;
    std::shared_ptr<TextBody> textBody_;
    std::shared_ptr<ShapePr> shapePr_;
};

} // namespace drawing
} // namespace office
} // namespace zq
