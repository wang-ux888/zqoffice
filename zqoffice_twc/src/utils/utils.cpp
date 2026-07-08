// =============================================================================
// src/utils/utils.cpp
//
// zq::office::Utils 类实现
// =============================================================================
#include "utils.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <limits>

// 平台相关 UTF 转换
#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
  #include <iconv.h>
#endif

namespace zq {
namespace office {

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
const std::string Utils::kDay[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

const std::string Utils::kMonth[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

const float Utils::kDefaultCharacterWidth    = 7.0f;
const int   Utils::kDefaultColWidthInChar    = 8;
const int   Utils::kDefaultContentTextSz     = 11;
const float Utils::kDefaultDPI               = 72.0f;
const int   Utils::kDefaultRowHeightInPt     = 20;
const int   Utils::kDefaultTileTextSz        = 18;
const float Utils::kExcelSpacingRatio        = 1.2f;
const float Utils::kPowerpointSpacingRadio   = 1.2f;
const float Utils::kWordSpacingRatio         = 1.2f;

// ---------------------------------------------------------------------------
// 字符串转数值
// ---------------------------------------------------------------------------
double Utils::Str2Double(const std::string& s, bool* ok) {
    return Str2Double(s.c_str(), ok);
}

double Utils::Str2Double(const char* s, bool* ok) {
    if (ok) *ok = false;
    if (!s || !*s) return 0.0;
    errno = 0;
    char* end = nullptr;
    double v = std::strtod(s, &end);
    if (errno == ERANGE) return 0.0;
    // 跳过尾部空白
    while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (end && *end == '\0') {
        if (ok) *ok = true;
        return v;
    }
    return 0.0;
}

int Utils::Str2Int(const std::string& s, int base, bool* ok) {
    return Str2Int(s.c_str(), base, ok);
}

int Utils::Str2Int(const char* s, int base, bool* ok) {
    if (ok) *ok = false;
    if (!s || !*s) return 0;
    errno = 0;
    char* end = nullptr;
    long v = std::strtol(s, &end, base);
    if (errno == ERANGE) return 0;
    while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (end && *end == '\0') {
        if (v < std::numeric_limits<int>::min() ||
            v > std::numeric_limits<int>::max()) return 0;
        if (ok) *ok = true;
        return static_cast<int>(v);
    }
    return 0;
}

long Utils::Str2Long(const char* s, int base, bool* ok) {
    if (ok) *ok = false;
    if (!s || !*s) return 0L;
    errno = 0;
    char* end = nullptr;
    long v = std::strtol(s, &end, base);
    if (errno == ERANGE) return 0L;
    while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (end && *end == '\0') {
        if (ok) *ok = true;
        return v;
    }
    return 0L;
}

unsigned long Utils::Str2ULong(const char* s, int base, bool* ok) {
    if (ok) *ok = false;
    if (!s || !*s) return 0UL;
    errno = 0;
    char* end = nullptr;
    unsigned long v = std::strtoul(s, &end, base);
    if (errno == ERANGE) return 0UL;
    while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (end && *end == '\0') {
        if (ok) *ok = true;
        return v;
    }
    return 0UL;
}

bool Utils::TryParseDoubleStrict(const std::string& s, double* out) {
    if (s.empty()) return false;
    errno = 0;
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (errno == ERANGE) return false;
    if (end == s.c_str()) return false;
    while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (end && *end == '\0') {
        if (out) *out = v;
        return true;
    }
    return false;
}

bool Utils::TryParseInt32(const std::string& s, int32_t* out) {
    if (s.empty()) return false;
    errno = 0;
    char* end = nullptr;
    long v = std::strtol(s.c_str(), &end, 10);
    if (errno == ERANGE) return false;
    if (end == s.c_str()) return false;
    while (end && *end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (end && *end == '\0') {
        if (v < std::numeric_limits<int32_t>::min() ||
            v > std::numeric_limits<int32_t>::max()) return false;
        if (out) *out = static_cast<int32_t>(v);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// 字符串比较与查找
// ---------------------------------------------------------------------------
bool Utils::StringEqual(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return std::strcmp(a, b) == 0;
}

char* Utils::strnstr(char* s1, const char* s2, int n) {
    if (!s1 || !s2 || n <= 0) return nullptr;
    if (!*s2) return s1;
    int s2len = static_cast<int>(std::strlen(s2));
    if (s2len > n) return nullptr;
    int limit = n - s2len;
    for (int i = 0; i <= limit; ++i) {
        if (s1[i] == *s2 && std::strncmp(s1 + i, s2, s2len) == 0) {
            return s1 + i;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// 编码转换
// ---------------------------------------------------------------------------
std::string Utils::U16ToUtf8(const std::u16string& s) {
    if (s.empty()) return std::string();
    std::string out;
    out.reserve(s.size() * 2);
    for (std::size_t i = 0; i < s.size(); ++i) {
        std::uint32_t cp = static_cast<std::uint16_t>(s[i]);
        // 处理代理对（surrogate pair）
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < s.size()) {
            std::uint32_t lo = static_cast<std::uint16_t>(s[i + 1]);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                ++i;
            }
        }
        // 编码为 UTF-8
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

std::u16string Utils::Utf8ToU16(const std::string& s) {
    if (s.empty()) return std::u16string();
    std::u16string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ) {
        std::uint32_t cp = 0;
        unsigned char c = static_cast<unsigned char>(s[i]);
        int extra = 0;
        if (c <= 0x7F) { cp = c; extra = 0; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else { ++i; continue; } // 非法字节跳过
        if (i + extra >= s.size()) break;
        for (int j = 0; j < extra; ++j) {
            unsigned char cc = static_cast<unsigned char>(s[i + 1 + j]);
            if ((cc & 0xC0) != 0x80) { cp = 0xFFFD; extra = 0; break; }
            cp = (cp << 6) | (cc & 0x3F);
        }
        i += 1 + extra;
        if (cp <= 0xFFFF) {
            out.push_back(static_cast<char16_t>(cp));
        } else {
            cp -= 0x10000;
            out.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
        }
    }
    return out;
}

std::wstring Utils::Utf8ToW(const std::string& s) {
#if defined(_WIN32)
    if (s.empty()) return std::wstring();
    int req = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (req <= 1) return std::wstring();
    std::wstring out(req - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), req);
    return out;
#else
    // POSIX：wstring 是 32 位
    std::wstring out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ) {
        std::uint32_t cp = 0;
        unsigned char c = static_cast<unsigned char>(s[i]);
        int extra = 0;
        if (c <= 0x7F) { cp = c; extra = 0; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else { ++i; continue; }
        if (i + extra >= s.size()) break;
        for (int j = 0; j < extra; ++j) {
            unsigned char cc = static_cast<unsigned char>(s[i + 1 + j]);
            if ((cc & 0xC0) != 0x80) { cp = 0xFFFD; extra = 0; break; }
            cp = (cp << 6) | (cc & 0x3F);
        }
        i += 1 + extra;
        out.push_back(static_cast<wchar_t>(cp));
    }
    return out;
#endif
}

std::string Utils::WToUtf8(const std::wstring& s) {
#if defined(_WIN32)
    if (s.empty()) return std::string();
    int req = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1,
                                  nullptr, 0, nullptr, nullptr);
    if (req <= 1) return std::string();
    std::string out(req - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1,
                        out.data(), req, nullptr, nullptr);
    return out;
#else
    std::string out;
    out.reserve(s.size() * 2);
    for (wchar_t wc : s) {
        std::uint32_t cp = static_cast<std::uint32_t>(wc);
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
#endif
}

// ---------------------------------------------------------------------------
// XML 工具
// ---------------------------------------------------------------------------
std::string Utils::StripLeadingXmlDeclaration(const std::string_view& xml) {
    if (xml.empty()) return std::string();
    std::size_t pos = 0;
    // 跳过前导空白
    while (pos < xml.size() &&
           (xml[pos] == ' ' || xml[pos] == '\t' ||
            xml[pos] == '\r' || xml[pos] == '\n')) {
        ++pos;
    }
    // 检测 <?xml
    static const char kDecl[] = "<?xml";
    constexpr std::size_t kDeclLen = 5;
    if (pos + kDeclLen > xml.size()) {
        return std::string(xml);
    }
    if (std::strncmp(xml.data() + pos, kDecl, kDeclLen) != 0) {
        return std::string(xml);
    }
    // 找到 ?>
    std::size_t end = xml.find("?>", pos + kDeclLen);
    if (end == std::string_view::npos) {
        return std::string(xml);
    }
    end += 2; // 跳过 ?>
    // 跳过尾随空白
    while (end < xml.size() &&
           (xml[end] == ' ' || xml[end] == '\t' ||
            xml[end] == '\r' || xml[end] == '\n')) {
        ++end;
    }
    return std::string(xml.substr(end));
}

// ---------------------------------------------------------------------------
// 单位换算
// ---------------------------------------------------------------------------
int Utils::Twips2Pix(int twips) {
    // 1 inch = 1440 twips；1 inch = 96 px（屏幕 DPI）
    // pix = twips * 96 / 1440 = twips / 15
    // 四舍五入：负数向 -∞ 取整
    if (twips >= 0) {
        return (twips + 7) / 15;
    }
    return -(((-twips) + 7) / 15);
}

double Utils::convertOoxml2AwtAngle(double a, double b, double c) {
    // OOXML 椭圆弧使用起始角度与扫掠角度（弧度）
    // AWT Arc2D 使用起始角度与扩展角度（度）
    // 这里实现一个保守转换：度 = 弧度 * 180 / PI
    const double kRad2Deg = 180.0 / 3.14159265358979323846;
    return a * kRad2Deg + c * b;
}

std::vector<double> Utils::excuteArcTo(std::vector<double>& vals,
                                       double a, double b, double c, double d) {
    // 简化实现：将椭圆弧转换为端点坐标
    std::vector<double> result;
    if (vals.size() < 4) {
        result = {0.0, 0.0, 0.0, 0.0};
        return result;
    }
    // 起点
    double x1 = vals[0];
    double y1 = vals[1];
    // 终点（基于椭圆参数 a,b 和角度 c,d 计算）
    double cx = (vals[0] + vals[2]) / 2.0;
    double cy = (vals[1] + vals[3]) / 2.0;
    double rx = std::fabs(vals[2] - vals[0]) / 2.0;
    double ry = std::fabs(vals[3] - vals[1]) / 2.0;
    double x2 = cx + rx * std::cos(c);
    double y2 = cy + ry * std::sin(c);
    result = {x1, y1, x2, y2};
    (void)a; (void)b; (void)d;
    return result;
}

std::pair<double, double> Utils::excuteEcArcTo(
    std::vector<std::pair<double, double>>& vals,
    bool flag, double a, double b, double c) {
    // Excel Chart 专用弧线插值
    if (vals.empty()) return {0.0, 0.0};
    auto& last = vals.back();
    double x = last.first + a * std::cos(c);
    double y = last.second + b * std::sin(c);
    if (flag) { std::swap(x, y); }
    return {x, y};
}

// ---------------------------------------------------------------------------
// 转换结果解析
// ---------------------------------------------------------------------------
bool Utils::TryParseExportResultErrorCode(const char* s, int* code) {
    if (!s || !*s) return false;
    // 简单 JSON 解析：查找 "code":
    const char* p = std::strstr(s, "\"code\"");
    if (!p) return false;
    p += 6; // 跳过 "code"
    while (*p && (*p == ' ' || *p == '\t' || *p == ':')) ++p;
    if (!*p) return false;
    errno = 0;
    char* end = nullptr;
    long v = std::strtol(p, &end, 10);
    if (end == p || errno == ERANGE) return false;
    if (code) *code = static_cast<int>(v);
    return true;
}

} // namespace office
} // namespace zq
