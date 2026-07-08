// =============================================================================
// src/drawing/enums/prst_shape_type.cpp
//
// PrstShapeType 枚举字符串转换实现
//
// 使用静态映射表（std::unordered_map）实现 O(1) 平均查找：
//   - 字符串 → 枚举：解析 OOXML prst 属性时使用
//   - 枚举 → 字符串：写回 OOXML prst 属性时使用
// =============================================================================
#include "prst_shape_type.h"

#include <unordered_map>
#include <utility>

namespace zq {
namespace office {
namespace drawing {

namespace {

struct EnumEntry {
    PrstShapeType type;
    const char* name;
};

/// 枚举值 ↔ OOXML 字符串名称映射表
/// 与 ECMA-376 第 20.1.9.55 节 ST_ShapeType 完全一致
constexpr EnumEntry kEntries[] = {
    // --- 基本几何形状 ---
    { PrstShapeType::kRect,                    "rect" },
    { PrstShapeType::kRoundRect,               "roundRect" },
    { PrstShapeType::kRound1Rect,              "round1Rect" },
    { PrstShapeType::kRound2DiagRect,          "round2DiagRect" },
    { PrstShapeType::kRound2SameRect,          "round2SameRect" },
    { PrstShapeType::kSnip1Rect,               "snip1Rect" },
    { PrstShapeType::kSnip2DiagRect,           "snip2DiagRect" },
    { PrstShapeType::kSnip2SameRect,           "snip2SameRect" },
    { PrstShapeType::kSnipRoundRect,           "snipRoundRect" },
    { PrstShapeType::kEllipse,                 "ellipse" },
    { PrstShapeType::kTriangle,                "triangle" },
    { PrstShapeType::kRtTriangle,              "rtTriangle" },
    { PrstShapeType::kDiamond,                 "diamond" },
    { PrstShapeType::kParallelogram,           "parallelogram" },
    { PrstShapeType::kTrapezoid,               "trapezoid" },
    { PrstShapeType::kNonIsoscelesTrapezoid,   "nonIsoscelesTrapezoid" },
    { PrstShapeType::kPentagon,                "pentagon" },
    { PrstShapeType::kHexagon,                 "hexagon" },
    { PrstShapeType::kHeptagon,                "heptagon" },
    { PrstShapeType::kOctagon,                 "octagon" },
    { PrstShapeType::kDecagon,                 "decagon" },
    { PrstShapeType::kDodecagon,               "dodecagon" },
    { PrstShapeType::kStar4,                   "star4" },
    { PrstShapeType::kStar5,                   "star5" },
    { PrstShapeType::kStar6,                   "star6" },
    { PrstShapeType::kStar7,                   "star7" },
    { PrstShapeType::kStar8,                   "star8" },
    { PrstShapeType::kStar10,                  "star10" },
    { PrstShapeType::kStar12,                  "star12" },
    { PrstShapeType::kStar16,                  "star16" },
    { PrstShapeType::kStar24,                  "star24" },
    { PrstShapeType::kStar32,                  "star32" },
    { PrstShapeType::kPie,                     "pie" },
    { PrstShapeType::kPieWedge,                "pieWedge" },
    { PrstShapeType::kChordPie,                "chordPie" },
    { PrstShapeType::kArc,                     "arc" },
    { PrstShapeType::kBlockArc,                "blockArc" },
    { PrstShapeType::kDonut,                   "donut" },
    { PrstShapeType::kNoSmoking,               "noSmoking" },
    { PrstShapeType::kPlus,                    "plus" },
    { PrstShapeType::kPlaque,                  "plaque" },
    { PrstShapeType::kCan,                     "can" },
    { PrstShapeType::kFrame,                   "frame" },
    { PrstShapeType::kHalfFrame,               "halfFrame" },
    { PrstShapeType::kCorner,                  "corner" },
    { PrstShapeType::kBracePair,               "bracePair" },
    { PrstShapeType::kBracketPair,             "bracketPair" },
    { PrstShapeType::kLeftBrace,               "leftBrace" },
    { PrstShapeType::kLeftBracket,             "leftBracket" },
    { PrstShapeType::kRightBrace,              "rightBrace" },
    { PrstShapeType::kRightBracket,            "rightBracket" },
    { PrstShapeType::kSun,                     "sun" },
    { PrstShapeType::kMoon,                    "moon" },
    { PrstShapeType::kHeart,                   "heart" },
    { PrstShapeType::kLightningBolt,           "lightningBolt" },
    { PrstShapeType::kCloud,                   "cloud" },
    { PrstShapeType::kTeardrop,                "teardrop" },
    { PrstShapeType::kFunnel,                  "funnel" },
    { PrstShapeType::kGear6,                   "gear6" },
    { PrstShapeType::kGear9,                   "gear9" },

    // --- 箭头与方向 ---
    { PrstShapeType::kRightArrow,              "rightArrow" },
    { PrstShapeType::kLeftArrow,               "leftArrow" },
    { PrstShapeType::kUpArrow,                 "upArrow" },
    { PrstShapeType::kDownArrow,               "downArrow" },
    { PrstShapeType::kLeftRightArrow,          "leftRightArrow" },
    { PrstShapeType::kUpDownArrow,             "upDownArrow" },
    { PrstShapeType::kBentUpArrow,             "bentUpArrow" },
    { PrstShapeType::kBentArrow,               "bentArrow" },
    { PrstShapeType::kChevron,                 "chevron" },
    { PrstShapeType::kNotchedRightArrow,       "notchedRightArrow" },
    { PrstShapeType::kStripedRightArrow,       "stripedRightArrow" },
    { PrstShapeType::kQuadArrow,               "quadArrow" },
    { PrstShapeType::kLeftRightUpArrow,        "leftRightUpArrow" },
    { PrstShapeType::kLeftUpArrow,             "leftUpArrow" },
    { PrstShapeType::kUTurnArrow,              "utrArrow" },
    { PrstShapeType::kCircularArrow,           "circularArrow" },
    { PrstShapeType::kLeftCircularArrow,       "leftCircularArrow" },
    { PrstShapeType::kLeftRightCircularArrow,  "leftRightCircularArrow" },
    { PrstShapeType::kCurvedRightArrow,        "curvedRightArrow" },
    { PrstShapeType::kCurvedLeftArrow,         "curvedLeftArrow" },
    { PrstShapeType::kCurvedUpArrow,           "curvedUpArrow" },
    { PrstShapeType::kCurvedDownArrow,         "curvedDownArrow" },
    { PrstShapeType::kSwooshArrow,             "swooshArrow" },
    { PrstShapeType::kDownArrowCallout,        "downArrowCallout" },
    { PrstShapeType::kUpArrowCallout,          "upArrowCallout" },
    { PrstShapeType::kLeftArrowCallout,        "leftArrowCallout" },
    { PrstShapeType::kRightArrowCallout,       "rightArrowCallout" },
    { PrstShapeType::kLeftRightArrowCallout,   "leftRightArrowCallout" },
    { PrstShapeType::kUpDownArrowCallout,      "upDownArrowCallout" },
    { PrstShapeType::kQuadArrowCallout,        "quadArrowCallout" },

    // --- 标注（Callout）---
    { PrstShapeType::kWedgeRectCallout,        "wedgeRectCallout" },
    { PrstShapeType::kWedgeRoundRectCallout,   "wedgeRoundRectCallout" },
    { PrstShapeType::kWedgeEllipseCallout,     "wedgeEllipseCallout" },
    { PrstShapeType::kCallout1,                "callout1" },
    { PrstShapeType::kCallout2,                "callout2" },
    { PrstShapeType::kCallout3,                "callout3" },
    { PrstShapeType::kBorderCallout1,          "borderCallout1" },
    { PrstShapeType::kBorderCallout2,          "borderCallout2" },
    { PrstShapeType::kBorderCallout3,          "borderCallout3" },
    { PrstShapeType::kAccentCallout1,          "accentCallout1" },
    { PrstShapeType::kAccentCallout2,          "accentCallout2" },
    { PrstShapeType::kAccentCallout3,          "accentCallout3" },
    { PrstShapeType::kAccentBorderCallout1,    "accentBorderCallout1" },
    { PrstShapeType::kAccentBorderCallout2,    "accentBorderCallout2" },
    { PrstShapeType::kAccentBorderCallout3,    "accentBorderCallout3" },
    { PrstShapeType::kAccent1Callout1,         "accent1Callout1" },
    { PrstShapeType::kAccent1Callout2,         "accent1Callout2" },
    { PrstShapeType::kAccent1Callout3,         "accent1Callout3" },
    { PrstShapeType::kAccent2Callout1,         "accent2Callout1" },
    { PrstShapeType::kAccent2Callout2,         "accent2Callout2" },
    { PrstShapeType::kAccent2Callout3,         "accent2Callout3" },
    { PrstShapeType::kAccent3Callout1,         "accent3Callout1" },
    { PrstShapeType::kAccent3Callout2,         "accent3Callout2" },
    { PrstShapeType::kAccent3Callout3,         "accent3Callout3" },
    { PrstShapeType::kCloudCallout,            "cloudCallout" },

    // --- 流程图 ---
    { PrstShapeType::kFlowChartProcess,                "flowChartProcess" },
    { PrstShapeType::kFlowChartAlternateProcess,       "flowChartAlternateProcess" },
    { PrstShapeType::kFlowChartDecision,               "flowChartDecision" },
    { PrstShapeType::kFlowChartInputOutput,            "flowChartInputOutput" },
    { PrstShapeType::kFlowChartPredefinedProcess,      "flowChartPredefinedProcess" },
    { PrstShapeType::kFlowChartInternalStorage,        "flowChartInternalStorage" },
    { PrstShapeType::kFlowChartDocument,               "flowChartDocument" },
    { PrstShapeType::kFlowChartMultidocument,          "flowChartMultidocument" },
    { PrstShapeType::kFlowChartTerminator,             "flowChartTerminator" },
    { PrstShapeType::kFlowChartPreparation,            "flowChartPreparation" },
    { PrstShapeType::kFlowChartManualInput,            "flowChartManualInput" },
    { PrstShapeType::kFlowChartManualOperation,        "flowChartManualOperation" },
    { PrstShapeType::kFlowChartConnector,              "flowChartConnector" },
    { PrstShapeType::kFlowChartOffpageConnector,       "flowChartOffpageConnector" },
    { PrstShapeType::kFlowChartPunchedCard,            "flowChartPunchedCard" },
    { PrstShapeType::kFlowChartPunchedTape,            "flowChartPunchedTape" },
    { PrstShapeType::kFlowChartSummingJunction,        "flowChartSummingJunction" },
    { PrstShapeType::kFlowChartOr,                     "flowChartOr" },
    { PrstShapeType::kFlowChartCollate,                "flowChartCollate" },
    { PrstShapeType::kFlowChartSort,                   "flowChartSort" },
    { PrstShapeType::kFlowChartExtract,                "flowChartExtract" },
    { PrstShapeType::kFlowChartMerge,                  "flowChartMerge" },
    { PrstShapeType::kFlowChartStoredData,             "flowChartOfflineStorage" },
    { PrstShapeType::kFlowChartDelay,                  "flowChartDelay" },
    { PrstShapeType::kFlowChartSequentialAccessStorage,"flowChartMagneticTape" },
    { PrstShapeType::kFlowChartMagneticDrum,           "flowChartMagneticDrum" },
    { PrstShapeType::kFlowChartDirectAccessStorage,    "flowChartMagneticDisk" },
    { PrstShapeType::kFlowChartDisplay,                "flowChartDisplay" },

    // --- 连接器 ---
    { PrstShapeType::kStraightConnector1,      "straightConnector1" },
    { PrstShapeType::kBentConnector2,          "bentConnector2" },
    { PrstShapeType::kBentConnector3,          "bentConnector3" },
    { PrstShapeType::kBentConnector4,          "bentConnector4" },
    { PrstShapeType::kBentConnector5,          "bentConnector5" },
    { PrstShapeType::kCurvedConnector2,        "curvedConnector2" },
    { PrstShapeType::kCurvedConnector3,        "curvedConnector3" },
    { PrstShapeType::kCurvedConnector4,        "curvedConnector4" },
    { PrstShapeType::kCurvedConnector5,        "curvedConnector5" },
    { PrstShapeType::kLine,                    "line" },
    { PrstShapeType::kLineInv,                 "lineInv" },

    // --- 标签 ---
    { PrstShapeType::kRibbon,                  "ribbon" },
    { PrstShapeType::kRibbon2,                 "ribbon2" },
    { PrstShapeType::kEllipseRibbon,           "ellipseRibbon" },
    { PrstShapeType::kEllipseRibbon2,          "ellipseRibbon2" },
    { PrstShapeType::kLeftRightRibbon,         "leftRightRibbon" },
    { PrstShapeType::kHorizontalScroll,        "horizontalScroll" },
    { PrstShapeType::kVerticalScroll,          "verticalScroll" },
    { PrstShapeType::kFoldedCorner,            "foldedCorner" },

    // --- 几何装饰 ---
    { PrstShapeType::kBevel,                   "bevel" },
    { PrstShapeType::kCube,                    "cube" },
    { PrstShapeType::kSmileyFace,              "smileyFace" },
    { PrstShapeType::kIrregularSeal1,          "irregularSeal1" },
    { PrstShapeType::kIrregularSeal2,          "irregularSeal2" },
    { PrstShapeType::kWave,                    "wave" },
    { PrstShapeType::kDoubleWave,              "doubleWave" },
    { PrstShapeType::kChartPlus,               "chartPlus" },
    { PrstShapeType::kChartStar,               "chartStar" },
    { PrstShapeType::kChartX,                  "chartX" },

    // --- 标签修饰 ---
    { PrstShapeType::kCornerTabs,              "cornerTabs" },
    { PrstShapeType::kPlaqueTabs,              "plaqueTabs" },
    { PrstShapeType::kSquareTabs,              "squareTabs" },

    // --- 数学符号 ---
    { PrstShapeType::kMathPlus,                "mathPlus" },
    { PrstShapeType::kMathMinus,               "mathMinus" },
    { PrstShapeType::kMathMultiply,            "mathMultiply" },
    { PrstShapeType::kMathDivide,              "mathDivide" },
    { PrstShapeType::kMathEqual,               "mathEqual" },
    { PrstShapeType::kMathNotEqual,            "mathNotEqual" },

    // --- 动作按钮 ---
    { PrstShapeType::kActionButtonBackPrevious,"actionButtonBackPrevious" },
    { PrstShapeType::kActionButtonForwardNext, "actionButtonForwardNext" },
    { PrstShapeType::kActionButtonBeginning,   "actionButtonBeginning" },
    { PrstShapeType::kActionButtonEnd,         "actionButtonEnd" },
    { PrstShapeType::kActionButtonHome,        "actionButtonHome" },
    { PrstShapeType::kActionButtonReturn,      "actionButtonReturn" },
    { PrstShapeType::kActionButtonDocument,    "actionButtonDocument" },
    { PrstShapeType::kActionButtonSound,       "actionButtonSound" },
    { PrstShapeType::kActionButtonMovie,       "actionButtonMovie" },
    { PrstShapeType::kActionButtonHelp,        "actionButtonHelp" },
    { PrstShapeType::kActionButtonInformation, "actionButtonInformation" },
    { PrstShapeType::kActionButtonBlank,       "actionButtonBlank" },

    // --- Home Plate ---
    { PrstShapeType::kHomePlate,               "homePlate" },
};

/// 字符串 → 枚举映射（lazy init）
const std::unordered_map<std::string, PrstShapeType>& stringToEnumMap() {
    static const std::unordered_map<std::string, PrstShapeType> kMap = []() {
        std::unordered_map<std::string, PrstShapeType> m;
        m.reserve(sizeof(kEntries) / sizeof(kEntries[0]));
        for (const auto& e : kEntries) {
            m.emplace(e.name, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* prstShapeTypeToString(PrstShapeType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

PrstShapeType prstShapeTypeFromString(std::string_view s) {
    if (s.empty()) return PrstShapeType::kUnknown;
    const auto& m = stringToEnumMap();
    auto it = m.find(std::string(s));
    if (it == m.end()) return PrstShapeType::kUnknown;
    return it->second;
}

} // namespace drawing
} // namespace office
} // namespace zq
