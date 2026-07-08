// =============================================================================
// src/utils/geometry.h
//
// zq::office 几何类
//   Point / TTPoint / RRect / Matrix / Path
//
// 用于 OOXML DrawingML 与 Lynx Painter 之间的坐标/路径互转
// =============================================================================
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <limits>

#include "zq/office/types.h"

namespace zq {
namespace office {

// ---------------------------------------------------------------------------
// 2D 矩阵（仿射变换：[a c tx; b d ty; 0 0 1]）
// ---------------------------------------------------------------------------
class Matrix {
public:
    float a = 1.0f, b = 0.0f;
    float c = 0.0f, d = 1.0f;
    float tx = 0.0f, ty = 0.0f;

    constexpr Matrix() = default;
    constexpr Matrix(float a, float b, float c, float d, float tx, float ty)
        : a(a), b(b), c(c), d(d), tx(tx), ty(ty) {}

    /// 单位矩阵
    static Matrix identity() { return Matrix{}; }

    /// 平移矩阵
    static Matrix translate(float dx, float dy) {
        return Matrix(1, 0, 0, 1, dx, dy);
    }

    /// 缩放矩阵
    static Matrix scale(float sx, float sy) {
        return Matrix(sx, 0, 0, sy, 0, 0);
    }

    /// 旋转矩阵（弧度）
    static Matrix rotate(float rad);

    /// 倾斜矩阵
    static Matrix skew(float sx, float sy) {
        return Matrix(1, std::tan(sy), std::tan(sx), 1, 0, 0);
    }

    /// 矩阵乘法
    Matrix operator*(const Matrix& m) const;

    /// 矩阵乘法（自乘）
    Matrix& operator*=(const Matrix& m) { *this = *this * m; return *this; }

    /// 应用到点
    Point apply(Point p) const {
        return Point(a * p.x + c * p.y + tx, b * p.x + d * p.y + ty);
    }

    /// 求逆矩阵，不可逆返回单位矩阵
    Matrix inverted() const;

    /// 是否为单位矩阵
    bool isIdentity() const;
};

// ---------------------------------------------------------------------------
// 用于 DrawingML custGeom 与 Escher path 互转
// ---------------------------------------------------------------------------
class Path {
public:
    enum class SegmentType : std::uint8_t {
        MoveTo,
        LineTo,
        CubicBezierTo,   // 三次贝塞尔
        QuadraticBezierTo, // 二次贝塞尔
        Close,
    };

    struct Segment {
        SegmentType type;
        std::vector<Point> points;  // MoveTo/LineTo: 1 个点；BezierTo: 3 个点；Close: 0 个
    };

    Path() = default;

    void moveTo(float x, float y) {
        Segment s; s.type = SegmentType::MoveTo; s.points.push_back({x, y});
        segments_.push_back(std::move(s));
    }
    void lineTo(float x, float y) {
        Segment s; s.type = SegmentType::LineTo; s.points.push_back({x, y});
        segments_.push_back(std::move(s));
    }
    void cubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
        Segment s; s.type = SegmentType::CubicBezierTo;
        s.points.push_back({c1x, c1y});
        s.points.push_back({c2x, c2y});
        s.points.push_back({x, y});
        segments_.push_back(std::move(s));
    }
    void quadraticBezierTo(float cx, float cy, float x, float y) {
        Segment s; s.type = SegmentType::QuadraticBezierTo;
        s.points.push_back({cx, cy});
        s.points.push_back({x, y});
        segments_.push_back(std::move(s));
    }
    void close() {
        Segment s; s.type = SegmentType::Close;
        segments_.push_back(std::move(s));
    }

    bool empty() const { return segments_.empty(); }
    std::size_t size() const { return segments_.size(); }
    const std::vector<Segment>& segments() const { return segments_; }
    void clear() { segments_.clear(); }

    /// 用矩阵变换路径
    Path transformed(const Matrix& m) const;

    /// 计算路径包围盒
    Rect boundingBox() const;

    /// 转换为 SVG path 字符串
    /// M = moveto, L = lineto, C = cubic bezier, Q = quadratic, Z = close
    std::string toSvgPath() const;

    /// 从 SVG path 字符串解析（仅支持 M/L/C/Q/Z 命令）
    static Path fromSvgPath(const std::string& d);

private:
    std::vector<Segment> segments_;
};

// ---------------------------------------------------------------------------
// 几何工具函数
// ---------------------------------------------------------------------------

/// EMU → 像素（96 DPI）
inline float emuToPixel(std::int64_t emu) {
    return static_cast<float>(emu) * 96.0f / 914400.0f;
}

/// 像素 → EMU
inline std::int64_t pixelToEmu(float px) {
    return static_cast<std::int64_t>(px * 914400.0f / 96.0f);
}

/// EMU → 磅
inline float emuToPoint(std::int64_t emu) {
    return static_cast<float>(emu) / 12700.0f;
}

/// 磅 → EMU
inline std::int64_t pointToEmu(float pt) {
    return static_cast<std::int64_t>(pt * 12700.0f);
}

/// 缇 → 磅
inline float twipsToPoint(int twips) {
    return twips / 20.0f;
}

/// 磅 → 缇
inline int pointToTwips(float pt) {
    return static_cast<int>(pt * 20.0f);
}

/// 列字母（"A", "B", ..., "Z", "AA", ...）→ 列索引（0-based）
int columnLettersToIndex(const std::string& letters);

/// 列索引（0-based）→ 列字母
std::string columnIndexToLetters(int index);

} // namespace office
} // namespace zq
