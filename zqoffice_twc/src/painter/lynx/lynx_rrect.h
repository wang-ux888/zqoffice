// =============================================================================
// src/painter/lynx/lynx_rrect.h
//
// zq::office::painter::lynx::LynxRRect 类（圆角矩形）
//   四边（left/top/right/bottom）+ 宽高（width/height）+ 平移（offset）
//
// =============================================================================
#pragma once

namespace zq {
namespace office {
namespace painter {
namespace lynx {

/// 圆角矩形（Lynx 渲染框架适配）
/// 存储矩形的四边坐标，提供宽高与平移操作
class LynxRRect {
public:
    /// 默认构造：全零矩形
    LynxRRect();

    /// 构造指定四边
    LynxRRect(float left, float top, float right, float bottom);

    /// 拷贝构造
    LynxRRect(const LynxRRect& other) = default;

    /// 移动构造
    LynxRRect(LynxRRect&& other) noexcept = default;

    /// 析构
    ~LynxRRect() = default;

    /// 拷贝赋值
    LynxRRect& operator=(const LynxRRect& other) = default;

    /// 移动赋值
    LynxRRect& operator=(LynxRRect&& other) noexcept = default;

    // ----- 四边 -----

    /// 取左边
    float left() const { return left_; }
    /// 取上边
    float top() const { return top_; }
    /// 取右边
    float right() const { return right_; }
    /// 取下边
    float bottom() const { return bottom_; }

    // ----- 宽高 -----

    /// 取宽度
    float width() const { return right_ - left_; }
    /// 取高度
    float height() const { return bottom_ - top_; }

    // ----- 操作 -----

    /// 平移矩形
    /// @param dx  X 方向偏移
    /// @param dy  Y 方向偏移
    void offset(float dx, float dy);

    /// 设置四边
    void SetRect(float left, float top, float right, float bottom);

private:
    float left_;
    float top_;
    float right_;
    float bottom_;
};

}  // namespace lynx
}  // namespace painter
}  // namespace office
}  // namespace zq
