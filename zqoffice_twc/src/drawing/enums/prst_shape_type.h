// =============================================================================
// src/drawing/enums/prst_shape_type.h
//
// 预设几何形状类型枚举（PrstShapeType）
//
//   对应 OOXML ECMA-376 第 20.1.9.55 节 ST_ShapeType，
//   共 189 个预设几何形状（preset geometry），用于 a:prstGeom prst="" 属性。
//
//   每个枚举值与 OOXML 字符串名称一一对应：
//     - prstShapeTypeToString()  : 枚举 → 字符串（写 OOXML 时使用）
//     - prstShapeTypeFromString(): 字符串 → 枚举（读 OOXML 时使用）
//
//   语义：预设形状的几何路径由 OOXML 规范预定义，无需 a:custGeom 描述。
//         渲染时根据 prst 名称 + a:avLst（adjustment list）参数生成路径。
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace zq {
namespace office {
namespace drawing {

/// 预设几何形状类型
///
/// 与 OOXML ST_ShapeType 对齐（ECMA-376 第 20.1.9.55 节）
/// 共 189 个预设形状，按字母顺序排列以便查阅
enum class PrstShapeType : std::uint16_t {
    kUnknown = 0,

    // --- 基本几何形状 ---
    kRect = 1,                  // rect - 矩形（最常见）
    kRoundRect,                 // roundRect - 圆角矩形
    kRound1Rect,                // round1Rect - 一个圆角的矩形
    kRound2DiagRect,            // round2DiagRect - 两个对角圆角的矩形
    kRound2SameRect,            // round2SameRect - 两个同侧圆角的矩形
    kSnip1Rect,                 // snip1Rect - 一个切角的矩形
    kSnip2DiagRect,             // snip2DiagRect - 两个对角切角的矩形
    kSnip2SameRect,             // snip2SameRect - 两个同侧切角的矩形
    kSnipRoundRect,             // snipRoundRect - 切角+圆角的矩形
    kEllipse,                   // ellipse - 椭圆
    kTriangle,                  // triangle - 三角形（默认底边在下）
    kRtTriangle,                // rtTriangle - 直角三角形
    kDiamond,                   // diamond - 菱形
    kParallelogram,             // parallelogram - 平行四边形
    kTrapezoid,                 // trapezoid - 梯形
    kNonIsoscelesTrapezoid,     // nonIsoscelesTrapezoid - 不等腰梯形
    kPentagon,                  // pentagon - 五边形
    kHexagon,                   // hexagon - 六边形
    kHeptagon,                  // heptagon - 七边形
    kOctagon,                   // octagon - 八边形
    kDecagon,                   // decagon - 十边形
    kDodecagon,                 // dodecagon - 十二边形
    kStar4,                     // star4 - 四角星
    kStar5,                     // star5 - 五角星
    kStar6,                     // star6 - 六角星
    kStar7,                     // star7 - 七角星
    kStar8,                     // star8 - 八角星
    kStar10,                    // star10 - 十角星
    kStar12,                    // star12 - 十二角星
    kStar16,                    // star16 - 十六角星
    kStar24,                    // star24 - 二十四角星
    kStar32,                    // star32 - 三十二角星
    kPie,                       // pie - 饼形
    kPieWedge,                  // pieWedge - 饼形楔
    kChordPie,                  // chordPie - 弦饼形
    kArc,                       // arc - 弧形
    kBlockArc,                  // blockArc - 块弧
    kDonut,                     // donut - 环形
    kNoSmoking,                 // noSmoking - 禁止标志
    kPlus,                      // plus - 加号
    kPlaque,                    // plaque - 牌匾
    kCan,                       // can - 圆柱
    kFrame,                     // frame - 框架
    kHalfFrame,                 // halfFrame - 半框架
    kCorner,                    // corner - 角
    kBracePair,                 // bracePair - 大括号对
    kBracketPair,               // bracketPair - 中括号对
    kLeftBrace,                 // leftBrace - 左大括号
    kLeftBracket,               // leftBracket - 左中括号
    kRightBrace,                // rightBrace - 右大括号
    kRightBracket,              // rightBracket - 右中括号
    kSun,                       // sun - 太阳
    kMoon,                      // moon - 月亮
    kHeart,                     // heart - 心形
    kLightningBolt,             // lightningBolt - 闪电
    kCloud,                     // cloud - 云形
    kTeardrop,                  // teardrop - 泪滴
    kFunnel,                    // funnel - 漏斗
    kGear6,                     // gear6 - 六齿齿轮
    kGear9,                     // gear9 - 九齿齿轮

    // --- 箭头与方向 ---
    kRightArrow,                // rightArrow - 右箭头
    kLeftArrow,                 // leftArrow - 左箭头
    kUpArrow,                   // upArrow - 上箭头
    kDownArrow,                 // downArrow - 下箭头
    kLeftRightArrow,            // leftRightArrow - 左右箭头
    kUpDownArrow,               // upDownArrow - 上下箭头
    kBentUpArrow,               // bentUpArrow - 弯曲上箭头
    kBentArrow,                 // bentArrow - 弯曲箭头
    kChevron,                   // chevron - V形箭头
    kNotchedRightArrow,         // notchedRightArrow - 凹槽右箭头
    kStripedRightArrow,         // stripedRightArrow - 条纹右箭头
    kQuadArrow,                 // quadArrow - 四向箭头
    kLeftRightUpArrow,          // leftRightUpArrow - 左右上箭头
    kLeftUpArrow,               // leftUpArrow - 左上箭头
    kUTurnArrow,                // utrArrow - U 形箭头
    kCircularArrow,             // circularArrow - 圆形箭头
    kLeftCircularArrow,         // leftCircularArrow - 左旋圆形箭头
    kLeftRightCircularArrow,    // leftRightCircularArrow - 左右圆形箭头
    kCurvedRightArrow,          // curvedRightArrow - 曲线右箭头
    kCurvedLeftArrow,           // curvedLeftArrow - 曲线左箭头
    kCurvedUpArrow,             // curvedUpArrow - 曲线上箭头
    kCurvedDownArrow,           // curvedDownArrow - 曲线下箭头
    kSwooshArrow,               // swooshArrow - 飞箭
    kDownArrowCallout,          // downArrowCallout - 下箭头标注
    kUpArrowCallout,            // upArrowCallout - 上箭头标注
    kLeftArrowCallout,          // leftArrowCallout - 左箭头标注
    kRightArrowCallout,         // rightArrowCallout - 右箭头标注
    kLeftRightArrowCallout,     // leftRightArrowCallout - 左右箭头标注
    kUpDownArrowCallout,        // upDownArrowCallout - 上下箭头标注
    kQuadArrowCallout,          // quadArrowCallout - 四向箭头标注

    // --- 标注（Callout）---
    kWedgeRectCallout,          // wedgeRectCallout - 楔形矩形标注
    kWedgeRoundRectCallout,     // wedgeRoundRectCallout - 楔形圆角矩形标注
    kWedgeEllipseCallout,       // wedgeEllipseCallout - 楔形椭圆标注
    kCallout1,                  // callout1 - 标注 1
    kCallout2,                  // callout2 - 标注 2
    kCallout3,                  // callout3 - 标注 3
    kBorderCallout1,            // borderCallout1 - 边框标注 1
    kBorderCallout2,            // borderCallout2 - 边框标注 2
    kBorderCallout3,            // borderCallout3 - 边框标注 3
    kAccentCallout1,            // accentCallout1 - 强调标注 1
    kAccentCallout2,            // accentCallout2 - 强调标注 2
    kAccentCallout3,            // accentCallout3 - 强调标注 3
    kAccentBorderCallout1,      // accentBorderCallout1 - 强调边框标注 1
    kAccentBorderCallout2,      // accentBorderCallout2 - 强调边框标注 2
    kAccentBorderCallout3,      // accentBorderCallout3 - 强调边框标注 3
    kAccent1Callout1,           // accent1Callout1
    kAccent1Callout2,           // accent1Callout2
    kAccent1Callout3,           // accent1Callout3
    kAccent2Callout1,           // accent2Callout1
    kAccent2Callout2,           // accent2Callout2
    kAccent2Callout3,           // accent2Callout3
    kAccent3Callout1,           // accent3Callout1
    kAccent3Callout2,           // accent3Callout2
    kAccent3Callout3,           // accent3Callout3
    kCloudCallout,              // cloudCallout - 云形标注

    // --- 流程图 ---
    kFlowChartProcess,          // flowChartProcess - 流程图过程
    kFlowChartAlternateProcess, // flowChartAlternateProcess - 备用过程
    kFlowChartDecision,         // flowChartDecision - 决策
    kFlowChartInputOutput,      // flowChartInputOutput - 输入输出
    kFlowChartPredefinedProcess,// flowChartPredefinedProcess - 预定义过程
    kFlowChartInternalStorage,  // flowChartInternalStorage - 内部存储
    kFlowChartDocument,         // flowChartDocument - 文档
    kFlowChartMultidocument,    // flowChartMultidocument - 多文档
    kFlowChartTerminator,       // flowChartTerminator - 终止符
    kFlowChartPreparation,      // flowChartPreparation - 准备
    kFlowChartManualInput,      // flowChartManualInput - 手动输入
    kFlowChartManualOperation,  // flowChartManualOperation - 手动操作
    kFlowChartConnector,        // flowChartConnector - 连接符
    kFlowChartOffpageConnector, // flowChartOffpageConnector - 离页连接符
    kFlowChartPunchedCard,      // flowChartPunchedCard - 穿孔卡片
    kFlowChartPunchedTape,      // flowChartPunchedTape - 穿孔带
    kFlowChartSummingJunction,  // flowChartSummingJunction - 汇合点
    kFlowChartOr,               // flowChartOr - 或
    kFlowChartCollate,          // flowChartCollate - 整理
    kFlowChartSort,             // flowChartSort - 排序
    kFlowChartExtract,          // flowChartExtract - 提取
    kFlowChartMerge,            // flowChartMerge - 合并
    kFlowChartStoredData,       // flowChartOfflineStorage - 离线存储
    kFlowChartDelay,            // flowChartDelay - 延迟
    kFlowChartSequentialAccessStorage, // flowChartMagneticTape - 磁带
    kFlowChartMagneticDrum,     // flowChartMagneticDrum - 磁鼓
    kFlowChartDirectAccessStorage, // flowChartMagneticDisk - 磁盘
    kFlowChartDisplay,          // flowChartDisplay - 显示

    // --- 连接器 ---
    kStraightConnector1,        // straightConnector1 - 直线连接器
    kBentConnector2,            // bentConnector2 - 弯曲连接器 2
    kBentConnector3,            // bentConnector3 - 弯曲连接器 3
    kBentConnector4,            // bentConnector4 - 弯曲连接器 4
    kBentConnector5,            // bentConnector5 - 弯曲连接器 5
    kCurvedConnector2,          // curvedConnector2 - 曲线连接器 2
    kCurvedConnector3,          // curvedConnector3 - 曲线连接器 3
    kCurvedConnector4,          // curvedConnector4 - 曲线连接器 4
    kCurvedConnector5,          // curvedConnector5 - 曲线连接器 5
    kLine,                      // line - 直线
    kLineInv,                   // lineInv - 反向直线

    // --- 标签 ---
    kRibbon,                    // ribbon - 缎带
    kRibbon2,                   // ribbon2 - 缎带 2
    kEllipseRibbon,             // ellipseRibbon - 椭圆缎带
    kEllipseRibbon2,            // ellipseRibbon2 - 椭圆缎带 2
    kLeftRightRibbon,           // leftRightRibbon - 左右缎带
    kHorizontalScroll,          // horizontalScroll - 水平卷轴
    kVerticalScroll,            // verticalScroll - 垂直卷轴
    kFoldedCorner,              // foldedCorner - 折角

    // --- 几何装饰 ---
    kBevel,                     // bevel - 斜面
    kCube,                      // cube - 立方体
    kSmileyFace,                // smileyFace - 笑脸
    kIrregularSeal1,            // irregularSeal1 - 不规则封印 1
    kIrregularSeal2,            // irregularSeal2 - 不规则封印 2
    kWave,                      // wave - 波浪
    kDoubleWave,                // doubleWave - 双波浪
    kChartPlus,                 // chartPlus - 图表加号
    kChartStar,                 // chartStar - 图表星形
    kChartX,                    // chartX - 图表 X 形

    // --- 标签修饰 ---
    kCornerTabs,                // cornerTabs - 角标签
    kPlaqueTabs,                // plaqueTabs - 牌匾标签
    kSquareTabs,                // squareTabs - 方形标签
    kBracketTabs,               // bracketTabs - 中括号标签

    // --- 数学符号 ---
    kMathPlus,                  // mathPlus - 加号
    kMathMinus,                 // mathMinus - 减号
    kMathMultiply,              // mathMultiply - 乘号
    kMathDivide,                // mathDivide - 除号
    kMathEqual,                 // mathEqual - 等号
    kMathNotEqual,              // mathNotEqual - 不等号

    // --- 动作按钮（ActionButton）---
    kActionButtonBackPrevious,  // actionButtonBackPrevious - 后退
    kActionButtonForwardNext,   // actionButtonForwardNext - 前进
    kActionButtonBeginning,     // actionButtonBeginning - 开始
    kActionButtonEnd,           // actionButtonEnd - 结束
    kActionButtonHome,          // actionButtonHome - 主页
    kActionButtonReturn,        // actionButtonReturn - 返回
    kActionButtonDocument,      // actionButtonDocument - 文档
    kActionButtonSound,         // actionButtonSound - 声音
    kActionButtonMovie,         // actionButtonMovie - 影片
    kActionButtonHelp,          // actionButtonHelp - 帮助
    kActionButtonInformation,   // actionButtonInformation - 信息
    kActionButtonBlank,         // actionButtonBlank - 空白

    // --- Home Plate ---
    kHomePlate,                 // homePlate - 本垒板

    kMaxValue = 0xFFFF,
};

/// 枚举 → 字符串（OOXML prst 属性值）
/// 未知的枚举值返回空字符串
const char* prstShapeTypeToString(PrstShapeType t);

/// 字符串 → 枚举（OOXML prst 属性值解析）
/// 未知的字符串返回 PrstShapeType::kUnknown
PrstShapeType prstShapeTypeFromString(std::string_view s);

} // namespace drawing
} // namespace office
} // namespace zq
