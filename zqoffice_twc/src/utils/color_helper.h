// =============================================================================
// src/utils/color_helper.h
//
// zq::office::ColorHelper 类
//   - SplitARGB  将 ARGB 整数拆分为 [A, R, G, B] 四个字节
// =============================================================================
#pragma once

#include <cstdint>
#include <array>

#include "zq/office/types.h"

namespace zq {
namespace office {

class ColorHelper {
public:
    /// 将 ARGB 整数拆分为 [A, R, G, B]
    /// @param argb  打包的 ARGB 整数（A 在最高字节）
    /// @return 数组 [alpha, red, green, blue]，每项 0-255
    static std::array<std::uint8_t, 4> SplitARGB(std::uint32_t argb);

    /// 将 [A, R, G, B] 打包为 ARGB 整数
    static std::uint32_t CombineARGB(std::uint8_t a,
                                     std::uint8_t r,
                                     std::uint8_t g,
                                     std::uint8_t b);

    /// 将 #RRGGBB 或 #AARRGGBB 十六进制字符串解析为 ARGB 整数
    /// @param hex  形如 "#FFFFFF" 或 "#FFFFFFFF"
    /// @param out  输出 ARGB 整数
    /// @return 解析成功返回 true
    static bool ParseHex(const std::string& hex, std::uint32_t* out);

    /// 将 ARGB 整数格式化为 "#AARRGGBB" 字符串
    static std::string ToHex(std::uint32_t argb);

    /// 将 ARGB 整数格式化为 "rgb(r, g, b)" 字符串（忽略 alpha）
    static std::string ToRgbCss(std::uint32_t argb);

    /// 将 ARGB 整数格式化为 "rgba(r, g, b, a)" 字符串
    static std::string ToRgbaCss(std::uint32_t argb);

    /// 计算两个颜色的亮度差（用于自动文字颜色选择）
    /// @return 0-255，值越大对比度越高
    static int ContrastRatio(std::uint32_t c1, std::uint32_t c2);

    /// 根据背景色自动选择前景色（黑或白）
    static std::uint32_t AutoFgColor(std::uint32_t bgColor);

    /// 颜色调和：将颜色按比例混合
    /// @param c1, c2  输入颜色
    /// @param ratio   c1 占比，范围 [0.0, 1.0]
    static std::uint32_t Mix(std::uint32_t c1, std::uint32_t c2, float ratio);
};

} // namespace office
} // namespace zq
