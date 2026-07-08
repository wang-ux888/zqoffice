// =============================================================================
// src/painter/painter_enums.h
//
// zq::office::painter 命名空间枚举
//   - Cap   线帽枚举（绘制路径端点样式）
//   - Join  连接枚举（绘制路径拐角样式）
//   - Style 样式枚举（绘制填充/描边模式）
// =============================================================================
#pragma once

namespace zq {
namespace office {
namespace painter {

/// 线帽枚举（绘制路径端点样式，对应 Skia Cap）
enum class Cap {
    kButt   = 0,  ///< 平头线帽（默认）
    kRound  = 1,  ///< 圆头线帽
    kSquare = 2,  ///< 方头线帽
};

/// 连接枚举（绘制路径拐角样式，对应 Skia Join）
enum class Join {
    kMiter = 0,  ///< 尖角连接（默认）
    kRound = 1,  ///< 圆角连接
    kBevel = 2,  ///< 斜角连接
};

/// 样式枚举（绘制填充/描边模式，对应 Skia Style）
enum class Style {
    kFill          = 0,  ///< 仅填充
    kStroke        = 1,  ///< 仅描边
    kFillAndStroke = 2,  ///< 填充 + 描边
};

}  // namespace painter
}  // namespace office
}  // namespace zq
