// =============================================================================
// src/drawing/enums/prst_clr_type.cpp
//
// PrstClrType 枚举字符串与 RGB 值转换实现
//
// RGB 值来自 OOXML ECMA-376 第 20.1.10.47 节 ST_PresetColorVal 规范附表
// =============================================================================
#include "prst_clr_type.h"

#include <unordered_map>
#include <utility>

namespace zq {
namespace office {
namespace drawing {

namespace {

struct ClrEntry {
    PrstClrType type;
    const char* name;
    const char* rgb;  // 6 位十六进制 RRGGBB
};

/// 颜色枚举 ↔ 名称 + RGB 映射表
/// RGB 值与 OOXML ECMA-376 第 20.1.10.47 节 ST_PresetColorVal 完全一致
constexpr ClrEntry kEntries[] = {
    // --- A ---
    { PrstClrType::kAliceBlue,            "aliceBlue",            "F0F8FF" },
    { PrstClrType::kAntiqueWhite,         "antiqueWhite",         "FAEBD7" },
    { PrstClrType::kAqua,                 "aqua",                 "00FFFF" },
    { PrstClrType::kAquaMarine,           "aquaMarine",           "7FFFD4" },
    { PrstClrType::kAzure,                "azure",                "F0FFFF" },

    // --- B ---
    { PrstClrType::kBeige,                "beige",                "F5F5DC" },
    { PrstClrType::kBisque,               "bisque",               "FFE4C4" },
    { PrstClrType::kBlack,                "black",                "000000" },
    { PrstClrType::kBlanchedAlmond,       "blanchedAlmond",       "FFEBCD" },
    { PrstClrType::kBlue,                 "blue",                 "0000FF" },
    { PrstClrType::kBlueViolet,           "blueViolet",           "8A2BE2" },
    { PrstClrType::kBrown,                "brown",                "A52A2A" },
    { PrstClrType::kBurlyWood,            "burlyWood",            "DEB887" },

    // --- C ---
    { PrstClrType::kCadetBlue,            "cadetBlue",            "5F9EA0" },
    { PrstClrType::kChartReuse,           "chartReuse",           "7FFF00" },
    { PrstClrType::kChocolate,            "chocolate",            "D2691E" },
    { PrstClrType::kCoral,                "coral",                "FF7F50" },
    { PrstClrType::kCornflowerBlue,       "cornflowerBlue",       "6495ED" },
    { PrstClrType::kCornsilk,             "cornsilk",             "FFF8DC" },
    { PrstClrType::kCrimson,              "crimson",              "DC143C" },
    { PrstClrType::kCyan,                 "cyan",                 "00FFFF" },

    // --- D ---
    { PrstClrType::kDarkBlue,             "darkBlue",             "00008B" },
    { PrstClrType::kDarkCyan,             "darkCyan",             "008B8B" },
    { PrstClrType::kDarkGoldenrod,        "darkGoldenrod",        "B8860B" },
    { PrstClrType::kDarkGray,             "darkGray",             "A9A9A9" },
    { PrstClrType::kDarkGreen,            "darkGreen",            "006400" },
    { PrstClrType::kDarkGrey,             "darkGrey",             "A9A9A9" },
    { PrstClrType::kDarkKhaki,            "darkKhaki",            "BDB76B" },
    { PrstClrType::kDarkMagenta,          "darkMagenta",          "8B008B" },
    { PrstClrType::kDarkOliveGreen,       "darkOliveGreen",       "556B2F" },
    { PrstClrType::kDarkOrange,           "darkOrange",           "FF8C00" },
    { PrstClrType::kDarkOrchid,           "darkOrchid",           "9932CC" },
    { PrstClrType::kDarkRed,              "darkRed",              "8B0000" },
    { PrstClrType::kDarkSalmon,           "darkSalmon",           "E9967A" },
    { PrstClrType::kDarkSeaGreen,         "darkSeaGreen",         "8FBC8B" },
    { PrstClrType::kDarkSlateBlue,        "darkSlateBlue",        "483D8B" },
    { PrstClrType::kDarkSlateGray,        "darkSlateGray",        "2F4F4F" },
    { PrstClrType::kDarkSlateGrey,        "darkSlateGrey",        "2F4F4F" },
    { PrstClrType::kDarkTurquoise,        "darkTurquoise",        "00CED1" },
    { PrstClrType::kDarkViolet,           "darkViolet",           "9400D3" },
    { PrstClrType::kDeepPink,             "deepPink",             "FF1493" },
    { PrstClrType::kDeepSkyBlue,          "deepSkyBlue",          "00BFFF" },
    { PrstClrType::kDimGray,              "dimGray",              "696969" },
    { PrstClrType::kDimGrey,              "dimGrey",              "696969" },
    { PrstClrType::kDodgerBlue,           "dodgerBlue",           "1E90FF" },

    // --- F ---
    { PrstClrType::kFirebrick,            "firebrick",            "B22222" },
    { PrstClrType::kFloralWhite,          "floralWhite",          "FFFAF0" },
    { PrstClrType::kForestGreen,          "forestGreen",          "228B22" },
    { PrstClrType::kFuchsia,              "fuchsia",              "FF00FF" },

    // --- G ---
    { PrstClrType::kGainsboro,            "gainsboro",            "DCDCDC" },
    { PrstClrType::kGhostWhite,           "ghostWhite",           "F8F8FF" },
    { PrstClrType::kGold,                 "gold",                 "FFD700" },
    { PrstClrType::kGoldenrod,            "goldenrod",            "DAA520" },
    { PrstClrType::kGray,                 "gray",                 "808080" },
    { PrstClrType::kGreen,                "green",                "008000" },
    { PrstClrType::kGreenYellow,          "greenYellow",          "ADFF2F" },
    { PrstClrType::kGrey,                 "grey",                 "808080" },

    // --- H ---
    { PrstClrType::kHoneydew,             "honeydew",             "F0FFF0" },
    { PrstClrType::kHotPink,              "hotPink",              "FF69B4" },

    // --- I ---
    { PrstClrType::kIndianRed,            "indianRed",            "CD5C5C" },
    { PrstClrType::kIndigo,               "indigo",               "4B0082" },
    { PrstClrType::kIvory,                "ivory",                "FFFFF0" },

    // --- K ---
    { PrstClrType::kKhaki,                "khaki",                "F0E68C" },

    // --- L ---
    { PrstClrType::kLavender,             "lavender",             "E6E6FA" },
    { PrstClrType::kLavenderBlush,        "lavenderBlush",        "FFF0F5" },
    { PrstClrType::kLawnGreen,            "lawnGreen",            "7CFC00" },
    { PrstClrType::kLemonChiffon,         "lemonChiffon",         "FFFACD" },
    { PrstClrType::kLightBlue,            "lightBlue",            "ADD8E6" },
    { PrstClrType::kLightCoral,           "lightCoral",           "F08080" },
    { PrstClrType::kLightCyan,            "lightCyan",            "E0FFFF" },
    { PrstClrType::kLightGoldenrodYellow, "lightGoldenrodYellow", "FAFAD2" },
    { PrstClrType::kLightGray,            "lightGray",            "D3D3D3" },
    { PrstClrType::kLightGreen,           "lightGreen",           "90EE90" },
    { PrstClrType::kLightGrey,            "lightGrey",            "D3D3D3" },
    { PrstClrType::kLightPink,            "lightPink",            "FFB6C1" },
    { PrstClrType::kLightSalmon,          "lightSalmon",          "FFA07A" },
    { PrstClrType::kLightSeaGreen,        "lightSeaGreen",        "20B2AA" },
    { PrstClrType::kLightSkyBlue,         "lightSkyBlue",         "87CEFA" },
    { PrstClrType::kLightSlateGray,       "lightSlateGray",       "778899" },
    { PrstClrType::kLightSlateGrey,       "lightSlateGrey",       "778899" },
    { PrstClrType::kLightSteelBlue,       "lightSteelBlue",       "B0C4DE" },
    { PrstClrType::kLightYellow,          "lightYellow",          "FFFFE0" },
    { PrstClrType::kLime,                 "lime",                 "00FF00" },
    { PrstClrType::kLimeGreen,            "limeGreen",            "32CD32" },
    { PrstClrType::kLinen,                "linen",                "FAF0E6" },

    // --- M ---
    { PrstClrType::kMagenta,              "magenta",              "FF00FF" },
    { PrstClrType::kMaroon,               "maroon",               "800000" },
    { PrstClrType::kMediumAquamarine,     "mediumAquamarine",     "66CDAA" },
    { PrstClrType::kMediumBlue,           "mediumBlue",           "0000CD" },
    { PrstClrType::kMediumOrchid,         "mediumOrchid",         "BA55D3" },
    { PrstClrType::kMediumPurple,         "mediumPurple",         "9370DB" },
    { PrstClrType::kMediumSeaGreen,       "mediumSeaGreen",       "3CB371" },
    { PrstClrType::kMediumSlateBlue,      "mediumSlateBlue",      "7B68EE" },
    { PrstClrType::kMediumSpringGreen,    "mediumSpringGreen",    "00FA9A" },
    { PrstClrType::kMediumTurquoise,      "mediumTurquoise",      "48D1CC" },
    { PrstClrType::kMediumVioletRed,      "mediumVioletRed",      "C71585" },
    { PrstClrType::kMidnightBlue,         "midnightBlue",         "191970" },
    { PrstClrType::kMintCream,            "mintCream",            "F5FFFA" },
    { PrstClrType::kMistyRose,            "mistyRose",            "FFE4E1" },
    { PrstClrType::kMoccasin,             "moccasin",             "FFE4B5" },

    // --- N ---
    { PrstClrType::kNavajoWhite,          "navajoWhite",          "FFDEAD" },
    { PrstClrType::kNavy,                 "navy",                 "000080" },

    // --- O ---
    { PrstClrType::kOldLace,              "oldLace",              "FDF5E6" },
    { PrstClrType::kOlive,                "olive",                "808000" },
    { PrstClrType::kOliveDrab,            "oliveDrab",            "6B8E23" },
    { PrstClrType::kOrange,               "orange",               "FFA500" },
    { PrstClrType::kOrangeRed,            "orangeRed",            "FF4500" },
    { PrstClrType::kOrchid,               "orchid",               "DA70D6" },

    // --- P ---
    { PrstClrType::kPaleGoldenrod,        "paleGoldenrod",        "EEE8AA" },
    { PrstClrType::kPaleGreen,            "paleGreen",            "98FB98" },
    { PrstClrType::kPaleTurquoise,        "paleTurquoise",        "AFEEEE" },
    { PrstClrType::kPaleVioletRed,        "paleVioletRed",        "DB7093" },
    { PrstClrType::kPapayaWhip,           "papayaWhip",           "FFEFD5" },
    { PrstClrType::kPeachPuff,            "peachPuff",            "FFDAB9" },
    { PrstClrType::kPeru,                 "peru",                 "CD853F" },
    { PrstClrType::kPink,                 "pink",                 "FFC0CB" },
    { PrstClrType::kPlum,                 "plum",                 "DDA0DD" },
    { PrstClrType::kPowderBlue,           "powderBlue",           "B0E0E6" },
    { PrstClrType::kPurple,               "purple",               "800080" },

    // --- R ---
    { PrstClrType::kRed,                  "red",                  "FF0000" },
    { PrstClrType::kRosyBrown,            "rosyBrown",            "BC8F8F" },
    { PrstClrType::kRoyalBlue,            "royalBlue",            "4169E1" },

    // --- S ---
    { PrstClrType::kSaddleBrown,          "saddleBrown",          "8B4513" },
    { PrstClrType::kSalmon,               "salmon",               "FA8072" },
    { PrstClrType::kSandyBrown,           "sandyBrown",           "F4A460" },
    { PrstClrType::kSeaGreen,             "seaGreen",             "2E8B57" },
    { PrstClrType::kSeaShell,             "seaShell",             "FFF5EE" },
    { PrstClrType::kSienna,               "sienna",               "A0522D" },
    { PrstClrType::kSilver,               "silver",               "C0C0C0" },
    { PrstClrType::kSkyBlue,              "skyBlue",              "87CEEB" },
    { PrstClrType::kSlateBlue,            "slateBlue",            "6A5ACD" },
    { PrstClrType::kSlateGray,            "slateGray",            "708090" },
    { PrstClrType::kSlateGrey,            "slateGrey",            "708090" },
    { PrstClrType::kSnow,                 "snow",                 "FFFAFA" },
    { PrstClrType::kSpringGreen,          "springGreen",          "00FF7F" },
    { PrstClrType::kSteelBlue,            "steelBlue",            "4682B4" },

    // --- T ---
    { PrstClrType::kTan,                  "tan",                  "D2B48C" },
    { PrstClrType::kTeal,                 "teal",                 "008080" },
    { PrstClrType::kThistle,              "thistle",              "D8BFD8" },
    { PrstClrType::kTomato,               "tomato",               "FF6347" },
    { PrstClrType::kTurquoise,            "turquoise",            "40E0D0" },

    // --- V ---
    { PrstClrType::kViolet,               "violet",               "EE82EE" },

    // --- W ---
    { PrstClrType::kWheat,                "wheat",                "F5DEB3" },
    { PrstClrType::kWhite,                "white",                "FFFFFF" },
    { PrstClrType::kWhiteSmoke,           "whiteSmoke",           "F5F5F5" },

    // --- Y ---
    { PrstClrType::kYellow,               "yellow",               "FFFF00" },
    { PrstClrType::kYellowGreen,          "yellowGreen",          "9ACD32" },
};

const std::unordered_map<std::string, PrstClrType>& stringToEnumMap() {
    static const std::unordered_map<std::string, PrstClrType> kMap = []() {
        std::unordered_map<std::string, PrstClrType> m;
        m.reserve(sizeof(kEntries) / sizeof(kEntries[0]));
        for (const auto& e : kEntries) {
            m.emplace(e.name, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* prstClrTypeToString(PrstClrType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

PrstClrType prstClrTypeFromString(std::string_view s) {
    if (s.empty()) return PrstClrType::kUnknown;
    const auto& m = stringToEnumMap();
    auto it = m.find(std::string(s));
    if (it == m.end()) return PrstClrType::kUnknown;
    return it->second;
}

const char* prstClrTypeToRgb(PrstClrType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.rgb;
    }
    return "";
}

} // namespace drawing
} // namespace office
} // namespace zq
