// =============================================================================
// src/painter/lynx/lynx_point.h
//
// zq::office::painter::lynx::LynxPoint 类
//   2D 点（float 坐标），支持 int/float 重载的 SetX/SetY
//
// =============================================================================
#pragma once

namespace zq {
namespace office {
namespace painter {
namespace lynx {

/// 2D 点（Lynx 渲染框架适配）
class LynxPoint {
public:
    /// 默认构造：(0, 0)
    LynxPoint();

    /// 构造指定坐标
    /// @param x  X 坐标
    /// @param y  Y 坐标
    LynxPoint(float x, float y);

    /// 拷贝构造
    LynxPoint(const LynxPoint& other) = default;

    /// 移动构造
    LynxPoint(LynxPoint&& other) noexcept = default;

    /// 析构
    ~LynxPoint() = default;

    /// 拷贝赋值
    LynxPoint& operator=(const LynxPoint& other) = default;

    /// 移动赋值
    LynxPoint& operator=(LynxPoint&& other) noexcept = default;

    // ----- Get -----

    /// 取 X 坐标
    float GetX() const { return x_; }
    /// 取 Y 坐标
    float GetY() const { return y_; }

    // ----- Set（int/float 重载） -----

    /// 设置 X 坐标（int 版本）
    void SetX(int x);
    /// 设置 X 坐标（float 版本）
    void SetX(float x);
    /// 设置 Y 坐标（int 版本）
    void SetY(int y);
    /// 设置 Y 坐标（float 版本）
    void SetY(float y);

    /// 同时设置 X/Y
    void Set(float x, float y);
    void Set(int x, int y);

private:
    float x_;
    float y_;
};

}  // namespace lynx
}  // namespace painter
}  // namespace office
}  // namespace zq
