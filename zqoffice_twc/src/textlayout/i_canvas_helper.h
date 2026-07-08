// =============================================================================
// src/textlayout/i_canvas_helper.h
//
// ICanvasHelper 接口（画布助手）
//
//   画布绘制抽象接口，由具体平台实现（如 GDI / Cairo / CoreGraphics / Lynx）。
//   ObjectRun::Draw(ICanvasHelper*, float) 通过此接口在画布上绘制对象。
//
//   本阶段仅定义接口，具体实现由后续 Phase（渲染层）提供。
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {
namespace textlayout {

/// 画布助手接口
class ICanvasHelper {
public:
    virtual ~ICanvasHelper() = default;

    /// 保存当前画布状态（变换矩阵、裁剪区域等）
    virtual void Save() = 0;

    /// 恢复上次 Save 时的画布状态
    virtual void Restore() = 0;

    /// 平移画布原点
    /// @param dx X 方向偏移（px）
    /// @param dy Y 方向偏移（px）
    virtual void Translate(float dx, float dy) = 0;

    /// 缩放画布
    /// @param sx X 方向缩放比
    /// @param sy Y 方向缩放比
    virtual void Scale(float sx, float sy) = 0;

    /// 裁剪矩形区域
    /// @param x 矩形左上角 X
    /// @param y 矩形左上角 Y
    /// @param width 矩形宽度
    /// @param height 矩形高度
    virtual void ClipRect(float x, float y, float width, float height) = 0;
};

} // namespace textlayout
} // namespace office
} // namespace zq
