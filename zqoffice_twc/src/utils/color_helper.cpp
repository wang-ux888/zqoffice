// =============================================================================
// src/utils/color_helper.cpp
// =============================================================================
#include "color_helper.h"

#include <cstdio>
#include <cctype>
#include <cmath>
#include <algorithm>

namespace zq {
namespace office {

std::array<std::uint8_t, 4> ColorHelper::SplitARGB(std::uint32_t argb) {
    return {
        static_cast<std::uint8_t>((argb >> 24) & 0xFF),  // A
        static_cast<std::uint8_t>((argb >> 16) & 0xFF),  // R
        static_cast<std::uint8_t>((argb >>  8) & 0xFF),  // G
        static_cast<std::uint8_t>((argb      ) & 0xFF),  // B
    };
}

std::uint32_t ColorHelper::CombineARGB(std::uint8_t a,
                                       std::uint8_t r,
                                       std::uint8_t g,
                                       std::uint8_t b) {
    return (static_cast<std::uint32_t>(a) << 24)
         | (static_cast<std::uint32_t>(r) << 16)
         | (static_cast<std::uint32_t>(g) <<  8)
         | (static_cast<std::uint32_t>(b));
}

bool ColorHelper::ParseHex(const std::string& hex, std::uint32_t* out) {
    if (hex.empty()) return false;
    std::string h = hex;
    // 去除前导 '#'
    if (h[0] == '#') h = h.substr(1);
    // 去除前导 '0x'
    if (h.size() >= 2 && (h[0] == '0') && (h[1] == 'x' || h[1] == 'X')) {
        h = h.substr(2);
    }
    if (h.empty()) return false;
    // 校验十六进制
    for (char c : h) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    // 补齐到 8 位
    if (h.size() == 6) {
        h = "FF" + h;  // 默认不透明
    } else if (h.size() == 3) {
        // 简写 #RGB → #RRGGBB
        h = std::string(2, h[0]) + std::string(2, h[1]) + std::string(2, h[2]);
        h = "FF" + h;
    } else if (h.size() != 8) {
        return false;
    }
    std::uint32_t v = 0;
    if (std::sscanf(h.c_str(), "%x", &v) != 1) return false;
    if (out) *out = v;
    return true;
}

std::string ColorHelper::ToHex(std::uint32_t argb) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%08X", argb);
    return std::string(buf);
}

std::string ColorHelper::ToRgbCss(std::uint32_t argb) {
    auto c = SplitARGB(argb);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "rgb(%u, %u, %u)",
                  static_cast<unsigned>(c[1]),
                  static_cast<unsigned>(c[2]),
                  static_cast<unsigned>(c[3]));
    return std::string(buf);
}

std::string ColorHelper::ToRgbaCss(std::uint32_t argb) {
    auto c = SplitARGB(argb);
    char buf[48];
    // CSS alpha 范围 0.0-1.0
    float a = c[0] / 255.0f;
    std::snprintf(buf, sizeof(buf), "rgba(%u, %u, %u, %.3f)",
                  static_cast<unsigned>(c[1]),
                  static_cast<unsigned>(c[2]),
                  static_cast<unsigned>(c[3]),
                  static_cast<double>(a));
    return std::string(buf);
}

// 计算相对亮度（WCAG 2.0）
static float relativeLuminance(std::uint8_t v) {
    float f = v / 255.0f;
    return (f <= 0.03928f) ? (f / 12.92f) : std::pow((f + 0.055f) / 1.055f, 2.4f);
}

int ColorHelper::ContrastRatio(std::uint32_t c1, std::uint32_t c2) {
    auto a = SplitARGB(c1);
    auto b = SplitARGB(c2);
    float l1 = 0.2126f * relativeLuminance(a[1])
             + 0.7152f * relativeLuminance(a[2])
             + 0.0722f * relativeLuminance(a[3]);
    float l2 = 0.2126f * relativeLuminance(b[1])
             + 0.7152f * relativeLuminance(b[2])
             + 0.0722f * relativeLuminance(b[3]);
    float ratio = (l1 + 0.05f) / (l2 + 0.05f);
    if (l2 > l1) ratio = (l2 + 0.05f) / (l1 + 0.05f);
    int r = static_cast<int>(std::round(ratio * 100.0f));
    if (r < 0) r = 0;
    if (r > 2100) r = 2100;  // 最大对比度 21:1
    return r;
}

std::uint32_t ColorHelper::AutoFgColor(std::uint32_t bgColor) {
    auto c = SplitARGB(bgColor);
    float l = 0.2126f * relativeLuminance(c[1])
            + 0.7152f * relativeLuminance(c[2])
            + 0.0722f * relativeLuminance(c[3]);
    // 阈值：亮度 > 0.5 选黑色，否则选白色
    if (l > 0.179f) {
        return 0xFF000000u;  // 黑
    }
    return 0xFFFFFFFFu;      // 白
}

std::uint32_t ColorHelper::Mix(std::uint32_t c1, std::uint32_t c2, float ratio) {
    // 语义：ratio = 0 → c1，ratio = 1 → c2（c1 → c2 的渐变进度）
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    auto a = SplitARGB(c1);
    auto b = SplitARGB(c2);
    float r1 = 1.0f - ratio;  // c1 占比
    float r2 = ratio;          // c2 占比
    return CombineARGB(
        static_cast<std::uint8_t>(std::round(a[0] * r1 + b[0] * r2)),
        static_cast<std::uint8_t>(std::round(a[1] * r1 + b[1] * r2)),
        static_cast<std::uint8_t>(std::round(a[2] * r1 + b[2] * r2)),
        static_cast<std::uint8_t>(std::round(a[3] * r1 + b[3] * r2)));
}

} // namespace office
} // namespace zq
