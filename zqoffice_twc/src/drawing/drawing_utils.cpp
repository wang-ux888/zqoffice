// =============================================================================
// src/drawing/drawing_utils.cpp
//
// DrawingUtils 实现
// =============================================================================
#include "drawing_utils.h"

#include "drawing/theme.h"
#include "drawing/text_body.h"

namespace zq {
namespace office {
namespace drawing {

namespace {

// ===========================================================================
// 静态映射表
// ===========================================================================

/// MSO_AUTO_SHAPE_TYPE 二进制码 ↔ PrstShapeType 映射表
/// 来源：[MS-ODRAW] 2.4.12 + Microsoft Office VBA Reference (MsoAutoShapeType)
struct MsoShapeEntry {
    std::uint32_t msoCode;
    PrstShapeType prst;
};

/// 常见 MSO 形状类型码（MsoAutoShapeType 枚举值）
/// 仅包含 PrstShapeType 枚举中实际存在的形状
constexpr MsoShapeEntry kPrstShapeTypeMap[] = {
    // 基本几何
    { 1,   PrstShapeType::kRect },
    { 2,   PrstShapeType::kRoundRect },
    { 3,   PrstShapeType::kEllipse },
    { 4,   PrstShapeType::kDiamond },
    { 5,   PrstShapeType::kTriangle },
    { 6,   PrstShapeType::kRtTriangle },
    { 7,   PrstShapeType::kParallelogram },
    { 8,   PrstShapeType::kTrapezoid },
    { 9,   PrstShapeType::kHexagon },
    { 10,  PrstShapeType::kOctagon },
    { 11,  PrstShapeType::kPentagon },
    // 星形
    { 12,  PrstShapeType::kStar4 },
    { 13,  PrstShapeType::kStar5 },
    { 14,  PrstShapeType::kStar6 },
    { 15,  PrstShapeType::kStar7 },
    { 16,  PrstShapeType::kStar8 },
    { 17,  PrstShapeType::kStar10 },
    { 18,  PrstShapeType::kStar12 },
    { 19,  PrstShapeType::kStar16 },
    { 20,  PrstShapeType::kStar24 },
    { 21,  PrstShapeType::kStar32 },
    // 块/饼/弧
    { 95,  PrstShapeType::kBevel },
    { 96,  PrstShapeType::kFrame },
    { 169, PrstShapeType::kBlockArc },
    { 170, PrstShapeType::kDonut },
    { 171, PrstShapeType::kNoSmoking },
    { 173, PrstShapeType::kPie },
    { 175, PrstShapeType::kArc },
    { 177, PrstShapeType::kChordPie },
    { 178, PrstShapeType::kTeardrop },
    { 180, PrstShapeType::kSun },
    { 181, PrstShapeType::kMoon },
    { 182, PrstShapeType::kHeart },
    { 183, PrstShapeType::kLightningBolt },
    { 184, PrstShapeType::kCloud },
    // 装饰
    { 75,  PrstShapeType::kPlus },
    { 76,  PrstShapeType::kPlaque },
    { 77,  PrstShapeType::kCan },
    { 78,  PrstShapeType::kDonut },
    { 79,  PrstShapeType::kArc },
    { 80,  PrstShapeType::kPie },
    { 92,  PrstShapeType::kFoldedCorner },
    // 几何装饰
    { 162, PrstShapeType::kWave },
    { 163, PrstShapeType::kDoubleWave },
    { 164, PrstShapeType::kCube },
    { 212, PrstShapeType::kSmileyFace },
    { 90,  PrstShapeType::kIrregularSeal1 },
    { 91,  PrstShapeType::kIrregularSeal2 },
    { 217, PrstShapeType::kChartPlus },
    { 243, PrstShapeType::kChartStar },
    { 244, PrstShapeType::kChartX },
    // 框架
    { 97,  PrstShapeType::kHalfFrame },
    { 98,  PrstShapeType::kCorner },
    { 99,  PrstShapeType::kCornerTabs },
    { 100, PrstShapeType::kPlaqueTabs },
    // 括号
    { 105, PrstShapeType::kLeftBrace },
    { 106, PrstShapeType::kRightBrace },
    { 107, PrstShapeType::kLeftBracket },
    { 108, PrstShapeType::kRightBracket },
    // 连接器
    { 109, PrstShapeType::kStraightConnector1 },
    { 110, PrstShapeType::kBentConnector2 },
    { 111, PrstShapeType::kBentConnector3 },
    { 112, PrstShapeType::kBentConnector4 },
    { 113, PrstShapeType::kBentConnector5 },
    { 114, PrstShapeType::kCurvedConnector2 },
    { 115, PrstShapeType::kCurvedConnector3 },
    { 116, PrstShapeType::kCurvedConnector4 },
    { 117, PrstShapeType::kCurvedConnector5 },
    // 标注
    { 118, PrstShapeType::kCallout1 },
    { 119, PrstShapeType::kCallout2 },
    { 120, PrstShapeType::kCallout3 },
    { 121, PrstShapeType::kAccentCallout1 },
    { 122, PrstShapeType::kAccentCallout2 },
    { 123, PrstShapeType::kAccentCallout3 },
    { 124, PrstShapeType::kBorderCallout1 },
    { 125, PrstShapeType::kBorderCallout2 },
    { 126, PrstShapeType::kBorderCallout3 },
    { 127, PrstShapeType::kAccentBorderCallout1 },
    { 128, PrstShapeType::kAccentBorderCallout2 },
    { 129, PrstShapeType::kAccentBorderCallout3 },
    { 67,  PrstShapeType::kWedgeRectCallout },
    { 68,  PrstShapeType::kWedgeRoundRectCallout },
    { 69,  PrstShapeType::kWedgeEllipseCallout },
    { 70,  PrstShapeType::kCloudCallout },
    // 缎带
    { 130, PrstShapeType::kRibbon },
    { 131, PrstShapeType::kRibbon2 },
    { 132, PrstShapeType::kEllipseRibbon },
    { 133, PrstShapeType::kEllipseRibbon2 },
    { 134, PrstShapeType::kLeftRightRibbon },
    // 箭头
    { 135, PrstShapeType::kLeftRightUpArrow },
    { 136, PrstShapeType::kLeftUpArrow },
    { 137, PrstShapeType::kUpArrow },
    { 138, PrstShapeType::kDownArrow },
    { 139, PrstShapeType::kLeftArrow },
    { 140, PrstShapeType::kRightArrow },
    { 141, PrstShapeType::kLeftRightArrow },
    { 142, PrstShapeType::kUpDownArrow },
    { 143, PrstShapeType::kQuadArrow },
    { 144, PrstShapeType::kLeftArrowCallout },
    { 145, PrstShapeType::kRightArrowCallout },
    { 146, PrstShapeType::kUpArrowCallout },
    { 147, PrstShapeType::kDownArrowCallout },
    { 148, PrstShapeType::kLeftRightArrowCallout },
    { 149, PrstShapeType::kUpDownArrowCallout },
    { 150, PrstShapeType::kQuadArrowCallout },
    { 151, PrstShapeType::kBentArrow },
    { 152, PrstShapeType::kUTurnArrow },
    { 153, PrstShapeType::kCircularArrow },
    { 154, PrstShapeType::kNotchedRightArrow },
    { 155, PrstShapeType::kHomePlate },
    { 156, PrstShapeType::kChevron },
    // 流程图
    { 197, PrstShapeType::kFlowChartProcess },
    { 158, PrstShapeType::kFlowChartAlternateProcess },
    { 198, PrstShapeType::kFlowChartDecision },
    { 199, PrstShapeType::kFlowChartInputOutput },
    { 200, PrstShapeType::kFlowChartPredefinedProcess },
    { 201, PrstShapeType::kFlowChartInternalStorage },
    { 202, PrstShapeType::kFlowChartDocument },
    { 203, PrstShapeType::kFlowChartMultidocument },
    { 204, PrstShapeType::kFlowChartTerminator },
    { 205, PrstShapeType::kFlowChartPreparation },
    { 206, PrstShapeType::kFlowChartManualInput },
    { 207, PrstShapeType::kFlowChartManualOperation },
    { 208, PrstShapeType::kFlowChartConnector },
    { 209, PrstShapeType::kFlowChartOffpageConnector },
    { 210, PrstShapeType::kFlowChartPunchedCard },
    { 211, PrstShapeType::kFlowChartPunchedTape },
    { 212, PrstShapeType::kFlowChartSummingJunction },
    { 213, PrstShapeType::kFlowChartOr },
    { 214, PrstShapeType::kFlowChartCollate },
    { 215, PrstShapeType::kFlowChartSort },
    { 216, PrstShapeType::kFlowChartExtract },
    { 217, PrstShapeType::kFlowChartMerge },
    { 218, PrstShapeType::kFlowChartStoredData },
    { 219, PrstShapeType::kFlowChartDelay },
    { 220, PrstShapeType::kFlowChartSequentialAccessStorage },
    { 221, PrstShapeType::kFlowChartMagneticDrum },
    { 222, PrstShapeType::kFlowChartDirectAccessStorage },
    { 223, PrstShapeType::kFlowChartDisplay },
    // 数学符号
    { 185, PrstShapeType::kMathPlus },
    { 186, PrstShapeType::kMathMinus },
    { 187, PrstShapeType::kMathMultiply },
    { 188, PrstShapeType::kMathDivide },
    { 189, PrstShapeType::kMathEqual },
    { 190, PrstShapeType::kMathNotEqual },
    // 动作按钮
    { 224, PrstShapeType::kActionButtonBlank },
    { 225, PrstShapeType::kActionButtonHome },
    { 226, PrstShapeType::kActionButtonHelp },
    { 227, PrstShapeType::kActionButtonInformation },
    { 228, PrstShapeType::kActionButtonForwardNext },
    { 229, PrstShapeType::kActionButtonBackPrevious },
    { 230, PrstShapeType::kActionButtonEnd },
    { 231, PrstShapeType::kActionButtonBeginning },
    { 232, PrstShapeType::kActionButtonReturn },
    { 233, PrstShapeType::kActionButtonDocument },
    { 234, PrstShapeType::kActionButtonSound },
    { 235, PrstShapeType::kActionButtonMovie },
    // 齿轮
    { 236, PrstShapeType::kGear6 },
    { 237, PrstShapeType::kGear9 },
    // 直线
    { 238, PrstShapeType::kLine },
};

/// Office 2003 颜色索引 ↔ PrstClrType 映射表
/// 旧版（Office 2003 及更早）使用 16 色调色板，索引 0x00-0x0F 为基本色
struct ColorIndexEntry {
    std::uint16_t index;
    PrstClrType type;
};

constexpr ColorIndexEntry kPrstClrTypeMap03[] = {
    // Office 2003 16 色标准调色板
    { 0x00, PrstClrType::kBlack },
    { 0x01, PrstClrType::kMaroon },
    { 0x02, PrstClrType::kGreen },
    { 0x03, PrstClrType::kOlive },
    { 0x04, PrstClrType::kNavy },
    { 0x05, PrstClrType::kPurple },
    { 0x06, PrstClrType::kTeal },
    { 0x07, PrstClrType::kGray },
    { 0x08, PrstClrType::kSilver },
    { 0x09, PrstClrType::kRed },
    { 0x0A, PrstClrType::kLime },
    { 0x0B, PrstClrType::kYellow },
    { 0x0C, PrstClrType::kBlue },
    { 0x0D, PrstClrType::kFuchsia },
    { 0x0E, PrstClrType::kAqua },
    { 0x0F, PrstClrType::kWhite },
    // 扩展调色板（索引 0x10-0x1F）
    { 0x10, PrstClrType::kDarkRed },
    { 0x11, PrstClrType::kDarkGreen },
    { 0x12, PrstClrType::kDarkBlue },
    { 0x13, PrstClrType::kDarkCyan },
    { 0x14, PrstClrType::kDarkMagenta },
    { 0x15, PrstClrType::kOlive },  // 0x15 = DarkYellow (#808000) → kOlive
    { 0x16, PrstClrType::kDarkGray },
    { 0x17, PrstClrType::kLightGray },
};

/// Office 2007 颜色索引 ↔ PrstClrType 映射表
/// 来自 [MS-ODRAW] 2.4.5 MSO_COLOR_ENUM
/// 注：PrstClrType 不含 Window/WindowText/Transparent 系统颜色，系统颜色由调用方
/// 在更上层处理（这里仅映射预设颜色枚举）
constexpr ColorIndexEntry kPrstClrTypeMap07[] = {
    // 系统颜色索引（0x80+）→ 等价 OOXML 预设颜色
    { 0x80, PrstClrType::kBlack },
    { 0x81, PrstClrType::kDarkGray },
    { 0x82, PrstClrType::kGreen },
    { 0x83, PrstClrType::kSilver },
    { 0x84, PrstClrType::kMaroon },
    { 0x85, PrstClrType::kWhite },
    { 0x86, PrstClrType::kBlack },
    { 0x87, PrstClrType::kBlack },
    { 0x88, PrstClrType::kBlack },
    { 0x89, PrstClrType::kWhite },
    { 0x8A, PrstClrType::kGray },
    { 0x8B, PrstClrType::kGray },
    { 0x8C, PrstClrType::kWhite },
    { 0x8D, PrstClrType::kGray },
    { 0x8E, PrstClrType::kBlack },
    { 0x8F, PrstClrType::kBlack },
    // 标准 OOXML 预设颜色（部分常用项）
    { 0x90, PrstClrType::kAliceBlue },
    { 0x91, PrstClrType::kAntiqueWhite },
    { 0x92, PrstClrType::kAqua },
    { 0x93, PrstClrType::kAquaMarine },
    { 0x94, PrstClrType::kAzure },
    { 0x95, PrstClrType::kBeige },
    { 0x96, PrstClrType::kBisque },
    { 0x97, PrstClrType::kBlack },
    { 0x98, PrstClrType::kBlanchedAlmond },
    { 0x99, PrstClrType::kBlue },
    { 0x9A, PrstClrType::kBlueViolet },
    { 0x9B, PrstClrType::kBrown },
    { 0x9C, PrstClrType::kBurlyWood },
    { 0x9D, PrstClrType::kCadetBlue },
    { 0x9E, PrstClrType::kChartReuse },
    { 0x9F, PrstClrType::kCoral },
    { 0xA0, PrstClrType::kCornflowerBlue },
    { 0xA1, PrstClrType::kCornsilk },
    { 0xA2, PrstClrType::kCrimson },
    { 0xA3, PrstClrType::kCyan },
};

/// 二进制 BuAutoNum 编码 ↔ BuAutoNumType 映射表
/// 来源：[MS-ODRAW] 2.4.10 MSO_BULLET_ENUM
struct BuAutoNumEntry {
    std::uint16_t code;
    BuAutoNumType type;
};

constexpr BuAutoNumEntry kBuAutoNumTypeMap[] = {
    // 字母编号（0x00-0x05）
    { 0x00, BuAutoNumType::kAlphaLcParenBoth },
    { 0x01, BuAutoNumType::kAlphaLcParenR },
    { 0x02, BuAutoNumType::kAlphaLcPeriod },
    { 0x03, BuAutoNumType::kAlphaUcParenBoth },
    { 0x04, BuAutoNumType::kAlphaUcParenR },
    { 0x05, BuAutoNumType::kAlphaUcPeriod },
    // 阿拉伯数字（0x06-0x09）
    { 0x06, BuAutoNumType::kArabicParenBoth },
    { 0x07, BuAutoNumType::kArabicParenR },
    { 0x08, BuAutoNumType::kArabicPeriod },
    { 0x09, BuAutoNumType::kArabicPlain },
    // 罗马数字（0x0A-0x0F）
    { 0x0A, BuAutoNumType::kRomanLcParenBoth },
    { 0x0B, BuAutoNumType::kRomanLcParenR },
    { 0x0C, BuAutoNumType::kRomanLcPeriod },
    { 0x0D, BuAutoNumType::kRomanUcParenBoth },
    { 0x0E, BuAutoNumType::kRomanUcParenR },
    { 0x0F, BuAutoNumType::kRomanUcPeriod },
    // 圆圈数字（0x10-0x11）
    { 0x10, BuAutoNumType::kCircleLcDbdPlain },
    { 0x11, BuAutoNumType::kCircleUcDbdPlain },
};

/// 主题调色板索引 ↔ ThemeType 映射
struct PaletteEntry {
    std::uint8_t index;
    ThemeType type;
};

constexpr PaletteEntry kPaletteMap[] = {
    { 0,  ThemeType::kDark1 },
    { 1,  ThemeType::kLight1 },
    { 2,  ThemeType::kDark2 },
    { 3,  ThemeType::kLight2 },
    { 4,  ThemeType::kAccent1 },
    { 5,  ThemeType::kAccent2 },
    { 6,  ThemeType::kAccent3 },
    { 7,  ThemeType::kAccent4 },
    { 8,  ThemeType::kAccent5 },
    { 9,  ThemeType::kAccent6 },
    { 10, ThemeType::kHyperlink },
    { 11, ThemeType::kFollowedHyperlink },
};

} // namespace

// ===========================================================================
// 公共方法实现
// ===========================================================================

PrstShapeType DrawingUtils::msoShapeTypeToPrst(std::uint32_t msoType) {
    for (const auto& e : kPrstShapeTypeMap) {
        if (e.msoCode == msoType) return e.prst;
    }
    return PrstShapeType::kUnknown;
}

std::uint32_t DrawingUtils::prstToMsoShapeType(PrstShapeType type) {
    for (const auto& e : kPrstShapeTypeMap) {
        if (e.prst == type) return e.msoCode;
    }
    return 0;
}

PrstClrType DrawingUtils::colorIndex03ToPrstClr(std::uint16_t idx) {
    for (const auto& e : kPrstClrTypeMap03) {
        if (e.index == idx) return e.type;
    }
    return PrstClrType::kUnknown;
}

PrstClrType DrawingUtils::colorIndex07ToPrstClr(std::uint16_t idx) {
    for (const auto& e : kPrstClrTypeMap07) {
        if (e.index == idx) return e.type;
    }
    return PrstClrType::kUnknown;
}

BuAutoNumType DrawingUtils::buAutoNumCodeToType(std::uint16_t code) {
    for (const auto& e : kBuAutoNumTypeMap) {
        if (e.code == code) return e.type;
    }
    return BuAutoNumType::kUnknown;
}

std::uint16_t DrawingUtils::buAutoNumTypeToCode(BuAutoNumType type) {
    for (const auto& e : kBuAutoNumTypeMap) {
        if (e.type == type) return e.code;
    }
    return 0;
}

std::string DrawingUtils::useHlinkClr(TextRun& run,
                                       const std::shared_ptr<Theme>& theme) {
    // 默认超链接颜色（与 Word/Excel/PPT 默认一致）
    std::string hlinkColor = defaultHlinkColor();

    // 从主题获取超链接颜色
    if (theme && theme->hasColor(ThemeType::kHyperlink)) {
        hlinkColor = theme->getColor(ThemeType::kHyperlink);
    }

    // 应用到文本运行
    run.fontColor = hlinkColor;
    // 注意：下划线由 RunPr 控制（textlayout 模块），此处不设置

    return hlinkColor;
}

std::uint8_t DrawingUtils::themeTypeToPaletteIndex(ThemeType type) {
    for (const auto& e : kPaletteMap) {
        if (e.type == type) return e.index;
    }
    return 0xFF;
}

ThemeType DrawingUtils::paletteIndexToThemeType(std::uint8_t idx) {
    for (const auto& e : kPaletteMap) {
        if (e.index == idx) return e.type;
    }
    return ThemeType::kUnknown;
}

const char* DrawingUtils::defaultHlinkColor() {
    return "0563c1";  // Office 默认超链接蓝色
}

} // namespace drawing
} // namespace office
} // namespace zq
