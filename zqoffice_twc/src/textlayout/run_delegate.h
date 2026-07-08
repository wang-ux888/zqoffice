// =============================================================================
// src/textlayout/run_delegate.h
//
// RunDelegate 接口（Run 委托）
//
//   对象 Run（如图片占位）的绘制委托，由具体业务层实现。
//   ObjectRun 持有 RunDelegate，在 Draw 时回调委托进行实际绘制。
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {
namespace textlayout {

class ICanvasHelper;

/// Run 委托接口
class RunDelegate {
public:
    virtual ~RunDelegate() = default;

    /// 绘制对象
    /// @param canvas 画布助手
    /// @param x X 坐标
    /// @param y Y 坐标
    /// @param width 宽度
    /// @param height 高度
    virtual void Draw(ICanvasHelper* canvas, float x, float y,
                      float width, float height) = 0;

    /// 对象宽度（px）
    virtual float GetWidth() const = 0;

    /// 对象高度（px）
    virtual float GetHeight() const = 0;
};

} // namespace textlayout
} // namespace office
} // namespace zq
