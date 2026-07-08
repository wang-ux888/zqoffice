// =============================================================================
// src/drawing/enums/prst_clr_type.h
//
// 预设颜色类型枚举（PrstClrType）
//
//   对应 OOXML ECMA-376 第 20.1.10.47 节 ST_PresetColorVal，
//   共 150 个预设颜色（preset color），用于 a:srgbClr/a:schemeClr 之外的
//   a:prstClr val="aliceBlue" 等场景。
//
//   每个枚举值对应一个固定的 RGB 值（来自 OOXML 规范附表）。
//   prstClrTypeToRgb()  : 枚举 → RRGGBB 字符串
//   prstClrTypeToString(): 枚举 → OOXML 名称
//   prstClrTypeFromString(): 字符串 → 枚举
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace zq {
namespace office {
namespace drawing {

/// 预设颜色类型
///
/// 与 OOXML ST_PresetColorVal 对齐（ECMA-376 第 20.1.10.47 节）
/// 共 150 个预设颜色，按字母顺序排列
enum class PrstClrType : std::uint8_t {
    kUnknown = 0,

    // --- A ---
    kAliceBlue,            // #F0F8FF
    kAntiqueWhite,         // #FAEBD7
    kAqua,                 // #00FFFF
    kAquaMarine,           // #7FFFD4
    kAzure,                // #F0FFFF

    // --- B ---
    kBeige,                // #F5F5DC
    kBisque,               // #FFE4C4
    kBlack,                // #000000
    kBlanchedAlmond,       // #FFEBCD
    kBlue,                 // #0000FF
    kBlueViolet,           // #8A2BE2
    kBrown,                // #A52A2A
    kBurlyWood,            // #DEB887

    // --- C ---
    kCadetBlue,            // #5F9EA0
    kChartReuse,           // #7FFF00
    kChocolate,            // #D2691E
    kCoral,                // #FF7F50
    kCornflowerBlue,       // #6495ED
    kCornsilk,             // #FFF8DC
    kCrimson,              // #DC143C
    kCyan,                 // #00FFFF

    // --- D ---
    kDarkBlue,             // #00008B
    kDarkCyan,             // #008B8B
    kDarkGoldenrod,        // #B8860B
    kDarkGray,             // #A9A9A9
    kDarkGreen,            // #006400
    kDarkGrey,             // #A9A9A9
    kDarkKhaki,            // #BDB76B
    kDarkMagenta,          // #8B008B
    kDarkOliveGreen,       // #556B2F
    kDarkOrange,           // #FF8C00
    kDarkOrchid,           // #9932CC
    kDarkRed,              // #8B0000
    kDarkSalmon,           // #E9967A
    kDarkSeaGreen,         // #8FBC8B
    kDarkSlateBlue,        // #483D8B
    kDarkSlateGray,        // #2F4F4F
    kDarkSlateGrey,        // #2F4F4F
    kDarkTurquoise,        // #00CED1
    kDarkViolet,           // #9400D3
    kDeepPink,             // #FF1493
    kDeepSkyBlue,          // #00BFFF
    kDimGray,              // #696969
    kDimGrey,              // #696969
    kDodgerBlue,           // #1E90FF

    // --- F ---
    kFirebrick,            // #B22222
    kFloralWhite,          // #FFFAF0
    kForestGreen,          // #228B22
    kFuchsia,              // #FF00FF

    // --- G ---
    kGainsboro,            // #DCDCDC
    kGhostWhite,           // #F8F8FF
    kGold,                 // #FFD700
    kGoldenrod,            // #DAA520
    kGray,                 // #808080
    kGreen,                // #008000
    kGreenYellow,          // #ADFF2F
    kGrey,                 // #808080

    // --- H ---
    kHoneydew,             // #F0FFF0
    kHotPink,              // #FF69B4

    // --- I ---
    kIndianRed,            // #CD5C5C
    kIndigo,               // #4B0082
    kIvory,                // #FFFFF0

    // --- K ---
    kKhaki,                // #F0E68C

    // --- L ---
    kLavender,             // #E6E6FA
    kLavenderBlush,        // #FFF0F5
    kLawnGreen,            // #7CFC00
    kLemonChiffon,         // #FFFACD
    kLightBlue,            // #ADD8E6
    kLightCoral,           // #F08080
    kLightCyan,            // #E0FFFF
    kLightGoldenrodYellow, // #FAFAD2
    kLightGray,            // #D3D3D3
    kLightGreen,           // #90EE90
    kLightGrey,            // #D3D3D3
    kLightPink,            // #FFB6C1
    kLightSalmon,          // #FFA07A
    kLightSeaGreen,        // #20B2AA
    kLightSkyBlue,         // #87CEFA
    kLightSlateGray,       // #778899
    kLightSlateGrey,       // #778899
    kLightSteelBlue,       // #B0C4DE
    kLightYellow,          // #FFFFE0
    kLime,                 // #00FF00
    kLimeGreen,            // #32CD32
    kLinen,                // #FAF0E6

    // --- M ---
    kMagenta,              // #FF00FF
    kMaroon,               // #800000
    kMediumAquamarine,     // #66CDAA
    kMediumBlue,           // #0000CD
    kMediumOrchid,         // #BA55D3
    kMediumPurple,         // #9370DB
    kMediumSeaGreen,       // #3CB371
    kMediumSlateBlue,      // #7B68EE
    kMediumSpringGreen,    // #00FA9A
    kMediumTurquoise,      // #48D1CC
    kMediumVioletRed,      // #C71585
    kMidnightBlue,         // #191970
    kMintCream,            // #F5FFFA
    kMistyRose,            // #FFE4E1
    kMoccasin,             // #FFE4B5

    // --- N ---
    kNavajoWhite,          // #FFDEAD
    kNavy,                 // #000080

    // --- O ---
    kOldLace,              // #FDF5E6
    kOlive,                // #808000
    kOliveDrab,            // #6B8E23
    kOrange,               // #FFA500
    kOrangeRed,            // #FF4500
    kOrchid,               // #DA70D6

    // --- P ---
    kPaleGoldenrod,        // #EEE8AA
    kPaleGreen,            // #98FB98
    kPaleTurquoise,        // #AFEEEE
    kPaleVioletRed,        // #DB7093
    kPapayaWhip,           // #FFEFD5
    kPeachPuff,            // #FFDAB9
    kPeru,                 // #CD853F
    kPink,                 // #FFC0CB
    kPlum,                 // #DDA0DD
    kPowderBlue,           // #B0E0E6
    kPurple,               // #800080

    // --- R ---
    kRed,                  // #FF0000
    kRosyBrown,            // #BC8F8F
    kRoyalBlue,            // #4169E1

    // --- S ---
    kSaddleBrown,          // #8B4513
    kSalmon,               // #FA8072
    kSandyBrown,           // #F4A460
    kSeaGreen,             // #2E8B57
    kSeaShell,             // #FFF5EE
    kSienna,               // #A0522D
    kSilver,               // #C0C0C0
    kSkyBlue,              // #87CEEB
    kSlateBlue,            // #6A5ACD
    kSlateGray,            // #708090
    kSlateGrey,            // #708090
    kSnow,                 // #FFFAFA
    kSpringGreen,          // #00FF7F
    kSteelBlue,            // #4682B4

    // --- T ---
    kTan,                  // #D2B48C
    kTeal,                 // #008080
    kThistle,              // #D8BFD8
    kTomato,               // #FF6347
    kTurquoise,            // #40E0D0

    // --- V ---
    kViolet,               // #EE82EE

    // --- W ---
    kWheat,                // #F5DEB3
    kWhite,                // #FFFFFF
    kWhiteSmoke,           // #F5F5F5

    // --- Y ---
    kYellow,               // #FFFF00
    kYellowGreen,          // #9ACD32
};

/// 枚举 → OOXML 字符串名称
const char* prstClrTypeToString(PrstClrType t);

/// 字符串 → 枚举（OOXML 颜色名称解析）
PrstClrType prstClrTypeFromString(std::string_view s);

/// 枚举 → RRGGBB 字符串（6 位十六进制，无 # 前缀）
/// 用于将预设颜色转换为 RGB 值
const char* prstClrTypeToRgb(PrstClrType t);

} // namespace drawing
} // namespace office
} // namespace zq
