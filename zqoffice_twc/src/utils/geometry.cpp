// =============================================================================
// src/utils/geometry.cpp
// =============================================================================
#include "geometry.h"

#include <cmath>
#include <cctype>
#include <cfloat>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <limits>

namespace zq {
namespace office {

// ---------------------------------------------------------------------------
// Matrix
// ---------------------------------------------------------------------------
Matrix Matrix::rotate(float rad) {
    float cs = std::cos(rad);
    float sn = std::sin(rad);
    return Matrix(cs, sn, -sn, cs, 0, 0);
}

Matrix Matrix::operator*(const Matrix& m) const {
    return Matrix(
        a * m.a + c * m.b,           // a
        b * m.a + d * m.b,           // b
        a * m.c + c * m.d,           // c
        b * m.c + d * m.d,           // d
        a * m.tx + c * m.ty + tx,    // tx
        b * m.tx + d * m.ty + ty);   // ty
}

Matrix Matrix::inverted() const {
    float det = a * d - b * c;
    if (std::fabs(det) < 1e-9f) return Matrix::identity();
    float inv = 1.0f / det;
    return Matrix(
         d * inv, -b * inv,
        -c * inv,  a * inv,
        (c * ty - d * tx) * inv,
        (b * tx - a * ty) * inv);
}

bool Matrix::isIdentity() const {
    return a == 1.0f && b == 0.0f && c == 0.0f
        && d == 1.0f && tx == 0.0f && ty == 0.0f;
}

// ---------------------------------------------------------------------------
// Path
// ---------------------------------------------------------------------------
Path Path::transformed(const Matrix& m) const {
    Path result;
    for (const auto& s : segments_) {
        Segment ns = s;
        for (auto& p : ns.points) {
            p = m.apply(p);
        }
        result.segments_.push_back(std::move(ns));
    }
    return result;
}

Rect Path::boundingBox() const {
    if (segments_.empty()) return Rect{};
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    for (const auto& s : segments_) {
        for (const auto& p : s.points) {
            minX = std::min(minX, p.x);
            minY = std::min(minY, p.y);
            maxX = std::max(maxX, p.x);
            maxY = std::max(maxY, p.y);
        }
    }
    return Rect(minX, minY, maxX - minX, maxY - minY);
}

std::string Path::toSvgPath() const {
    std::ostringstream os;
    char buf[128];
    for (const auto& s : segments_) {
        switch (s.type) {
        case SegmentType::MoveTo:
            std::snprintf(buf, sizeof(buf), "M %.4f %.4f ", s.points[0].x, s.points[0].y);
            os << buf;
            break;
        case SegmentType::LineTo:
            std::snprintf(buf, sizeof(buf), "L %.4f %.4f ", s.points[0].x, s.points[0].y);
            os << buf;
            break;
        case SegmentType::CubicBezierTo:
            std::snprintf(buf, sizeof(buf),
                "C %.4f %.4f %.4f %.4f %.4f %.4f ",
                s.points[0].x, s.points[0].y,
                s.points[1].x, s.points[1].y,
                s.points[2].x, s.points[2].y);
            os << buf;
            break;
        case SegmentType::QuadraticBezierTo:
            std::snprintf(buf, sizeof(buf),
                "Q %.4f %.4f %.4f %.4f ",
                s.points[0].x, s.points[0].y,
                s.points[1].x, s.points[1].y);
            os << buf;
            break;
        case SegmentType::Close:
            os << "Z ";
            break;
        }
    }
    return os.str();
}

// 简单 SVG path 解析器：支持 M/L/C/Q/Z 命令（绝对坐标）
static std::vector<float> parseNumbers(const std::string& s, std::size_t& i) {
    std::vector<float> nums;
    while (i < s.size()) {
        // 跳过空白与逗号
        while (i < s.size() && (std::isspace(static_cast<unsigned char>(s[i])) || s[i] == ',')) ++i;
        if (i >= s.size()) break;
        // 检查是否还是数字
        char c = s[i];
        if (!(c == '-' || c == '+' || c == '.' ||
              (c >= '0' && c <= '9'))) break;
        char* end = nullptr;
        float v = std::strtof(s.c_str() + i, &end);
        if (end == s.c_str() + i) break;
        nums.push_back(v);
        i = static_cast<std::size_t>(end - s.c_str());
    }
    return nums;
}

Path Path::fromSvgPath(const std::string& d) {
    Path p;
    std::size_t i = 0;
    while (i < d.size()) {
        char cmd = d[i];
        if (std::isspace(static_cast<unsigned char>(cmd))) { ++i; continue; }
        ++i;  // 跳过命令字符
        switch (cmd) {
        case 'M':
        case 'm': {
            auto nums = parseNumbers(d, i);
            for (std::size_t k = 0; k + 1 < nums.size(); k += 2) {
                p.moveTo(nums[k], nums[k + 1]);
            }
            break;
        }
        case 'L':
        case 'l': {
            auto nums = parseNumbers(d, i);
            for (std::size_t k = 0; k + 1 < nums.size(); k += 2) {
                p.lineTo(nums[k], nums[k + 1]);
            }
            break;
        }
        case 'C':
        case 'c': {
            auto nums = parseNumbers(d, i);
            for (std::size_t k = 0; k + 5 < nums.size(); k += 6) {
                p.cubicBezierTo(nums[k], nums[k + 1],
                                nums[k + 2], nums[k + 3],
                                nums[k + 4], nums[k + 5]);
            }
            break;
        }
        case 'Q':
        case 'q': {
            auto nums = parseNumbers(d, i);
            for (std::size_t k = 0; k + 3 < nums.size(); k += 4) {
                p.quadraticBezierTo(nums[k], nums[k + 1],
                                    nums[k + 2], nums[k + 3]);
            }
            break;
        }
        case 'Z':
        case 'z':
            p.close();
            break;
        default:
            // 未知命令，跳过一个字符避免死循环
            break;
        }
    }
    return p;
}

// ---------------------------------------------------------------------------
// Excel 列字母转换
// ---------------------------------------------------------------------------
int columnLettersToIndex(const std::string& letters) {
    if (letters.empty()) return -1;
    int idx = 0;
    for (char c : letters) {
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        if (c < 'A' || c > 'Z') return -1;
        idx = idx * 26 + (c - 'A' + 1);
    }
    return idx - 1;  // 0-based
}

std::string columnIndexToLetters(int index) {
    if (index < 0) return std::string();
    std::string s;
    int n = index + 1;  // 1-based
    while (n > 0) {
        int rem = (n - 1) % 26;
        s.insert(s.begin(), static_cast<char>('A' + rem));
        n = (n - 1) / 26;
    }
    return s;
}

} // namespace office
} // namespace zq
