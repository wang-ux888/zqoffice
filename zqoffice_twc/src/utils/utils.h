// =============================================================================
// src/utils/utils.h
//
// zq::office::Utils 类
//   - Str2Double / Str2Int / Str2Long / Str2ULong
//   - TryParseDoubleStrict / TryParseInt32
//   - StringEqual / strnstr
//   - U16ToUtf8 / Utf8ToU16 / Utf8ToW / WToUtf8
//   - StripLeadingXmlDeclaration
//   - Twips2Pix / convertOoxml2AwtAngle / excuteArcTo / excuteEcArcTo
//   - TryParseExportResultErrorCode
//   - 静态常量 kDay / kMonth / kDefault*
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <string_view>

namespace zq {
namespace office {

class Utils {
public:
    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    /// 字符串转 double（C 风格 strtod 兼容语义）
    /// @param ok  可选输出参数，转换成功置 true
    static double Str2Double(const std::string& s, bool* ok = nullptr);
    static double Str2Double(const char* s, bool* ok = nullptr);

    /// 字符串转 int（支持 2/8/10/16 进制，与 strtol 对齐）
    static int Str2Int(const std::string& s, int base = 10, bool* ok = nullptr);
    static int Str2Int(const char* s, int base = 10, bool* ok = nullptr);

    /// 字符串转 long
    static long Str2Long(const char* s, int base = 10, bool* ok = nullptr);

    /// 字符串转 unsigned long
    static unsigned long Str2ULong(const char* s, int base = 10, bool* ok = nullptr);

    /// 严格解析 double：必须是合法浮点格式，不允许尾随非数字字符
    static bool TryParseDoubleStrict(const std::string& s, double* out);

    /// 严格解析 int32：必须是合法整数格式，不允许尾随非数字字符
    static bool TryParseInt32(const std::string& s, int32_t* out);

    // -----------------------------------------------------------------------
    // 字符串比较与查找
    // -----------------------------------------------------------------------

    /// 字符串相等比较（nullptr 安全）
    static bool StringEqual(const char* a, const char* b);

    /// 在 s1 前 n 字节中查找子串 s2（与 BSD strnstr 语义一致）
    /// @return 找到则返回出现位置指针，否则 nullptr
    static char* strnstr(char* s1, const char* s2, int n);

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    /// UTF-16 字符串 → UTF-8
    static std::string U16ToUtf8(const std::u16string& s);

    /// UTF-8 字符串 → UTF-16
    static std::u16string Utf8ToU16(const std::string& s);

    /// UTF-8 → 系统宽字符串（Windows 下为 UTF-16，Unix 下为 UTF-32）
    static std::wstring Utf8ToW(const std::string& s);

    /// 系统宽字符串 → UTF-8
    static std::string WToUtf8(const std::wstring& s);

    // -----------------------------------------------------------------------
    // XML 工具
    // -----------------------------------------------------------------------

    /// 剥离 XML 头部的 <?xml version="1.0" ...?> 声明
    static std::string StripLeadingXmlDeclaration(const std::string_view& xml);

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    /// 缇（twip）→ 像素（基于 96 DPI）
    static int Twips2Pix(int twips);

    /// OOXML 角度（弧度）→ AWT 角度（度）
    /// 用于 ArcTo 椭圆弧参数转换
    /// @param a  椭圆弧参数 a
    /// @param b  椭圆弧参数 b
    /// @param c  椭圆弧参数 c
    /// @return 转换后的角度（度）
    static double convertOoxml2AwtAngle(double a, double b, double c);

    /// 执行 ArcTo 弧线插值
    /// @param vals  起始参数 [startX, startY, endX, endY]
    /// @param a, b, c  椭圆参数
    /// @return 转换后的坐标列表 [x1, y1, x2, y2]
    static std::vector<double> excuteArcTo(std::vector<double>& vals,
                                           double a, double b, double c, double d);

    /// 执行 EcArcTo 弧线插值（Excel Chart 专用）
    /// @param vals  坐标点列表
    /// @param flag  是否反转
    /// @param a, b, c  弧线参数
    /// @return 插值后的 [x, y] 坐标对
    static std::pair<double, double> excuteEcArcTo(
        std::vector<std::pair<double, double>>& vals,
        bool flag, double a, double b, double c);

    // -----------------------------------------------------------------------
    // 转换结果解析
    // -----------------------------------------------------------------------

    /// @param s  完整 JSON 结果字符串
    /// @param code  输出解析得到的错误码
    /// @return 解析成功返回 true
    static bool TryParseExportResultErrorCode(const char* s, int* code);

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    static const std::string kDay[];     // ["Sunday".."Saturday"]
    static const std::string kMonth[];   // ["January".."December"]

    static const float kDefaultCharacterWidth;
    static const int   kDefaultColWidthInChar;
    static const int   kDefaultContentTextSz;
    static const float kDefaultDPI;
    static const int   kDefaultRowHeightInPt;
    static const int   kDefaultTileTextSz;
    static const float kExcelSpacingRatio;
    static const float kPowerpointSpacingRadio;
    static const float kWordSpacingRatio;
};

} // namespace office
} // namespace zq
