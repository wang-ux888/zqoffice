// =============================================================================
// zq/office/constants.h
//
// ZQOffice 公共常量
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>

namespace zq {
namespace office {
namespace constants {

// ---------------------------------------------------------------------------
// 默认渲染参数（与 Utils::kDefault* 对齐）
// ---------------------------------------------------------------------------

inline constexpr float kDefaultDPI = 72.0f;

/// 默认字符宽度（Excel 单元格列宽计算用，单位 EMU）
inline constexpr float kDefaultCharacterWidth = 7.0f;

/// 默认列宽（字符数）
inline constexpr int kDefaultColWidthInChar = 8;

/// 默认行高（磅）
inline constexpr int kDefaultRowHeightInPt = 20;

/// 默认正文文字大小（磅）
inline constexpr int kDefaultContentTextSz = 11;

/// 默认标题文字大小（磅）
inline constexpr int kDefaultTileTextSz = 18;

// ---------------------------------------------------------------------------
// 行间距比例（与 Utils::kXxxSpacingRadio 对齐）
// ---------------------------------------------------------------------------

/// Excel 行间距比例
inline constexpr float kExcelSpacingRatio = 1.2f;

/// PowerPoint 行间距比例
inline constexpr float kPowerpointSpacingRadio = 1.2f;

/// Word 行间距比例
inline constexpr float kWordSpacingRatio = 1.2f;

// ---------------------------------------------------------------------------
// OOXML 单位换算
// ---------------------------------------------------------------------------

/// 1 磅 = 12700 EMU（English Metric Unit）
inline constexpr int kEmuPerPoint = 12700;

/// 1 英寸 = 914400 EMU
inline constexpr int kEmuPerInch = 914400;

/// 1 厘米 = 360000 EMU
inline constexpr int kEmuPerCm = 360000;

/// 1 毫米 = 36000 EMU
inline constexpr int kEmuPerMm = 36000;

/// 1 缇（twip）= 1/20 磅 = 635 EMU
inline constexpr int kEmuPerTwip = 635;

/// 1 缇 = 1/1440 英寸
inline constexpr int kTwipsPerInch = 1440;

/// 1 缇 = 1/20 磅
inline constexpr int kTwipsPerPoint = 20;

// ---------------------------------------------------------------------------
// BIFF / OLE2 / Escher 规范常量
// ---------------------------------------------------------------------------

/// OLE2 复合文档魔数：D0 CF 11 E0 A1 B1 1A E1
inline constexpr std::uint64_t kOle2Magic = 0xE11AB1A1E011CFD0ULL;

/// ZIP/OOXML 魔数：50 4B 03 04（"PK\x03\x04"）
inline constexpr std::uint32_t kZipMagic = 0x04034B50u;

/// OLE2 扇区大小（默认 512 字节）
inline constexpr int kOle2DefaultSectorSize = 512;

/// OLE2 Mini 扇区大小（默认 64 字节）
inline constexpr int kOle2DefaultMiniSectorSize = 64;

/// OLE2 Mini Stream Cutoff（默认 4096 字节，小于此值的流走 MiniFAT）
inline constexpr int kOle2MiniStreamCutoff = 4096;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

inline constexpr std::uint32_t kColorBlack   = 0xFF000000u;
inline constexpr std::uint32_t kColorWhite   = 0xFFFFFFFFu;
inline constexpr std::uint32_t kColorTransparent = 0x00000000u;
inline constexpr std::uint32_t kColorDefault = 0xFF000000u;  // 默认前景色

// ---------------------------------------------------------------------------
// 字符串与编码
// ---------------------------------------------------------------------------

/// UTF-8 BOM
inline constexpr const char* kUtf8Bom = "\xEF\xBB\xBF";

/// UTF-16 LE BOM
inline constexpr const char* kUtf16LeBom = "\xFF\xFE";

/// UTF-16 BE BOM
inline constexpr const char* kUtf16BeBom = "\xFE\xFF";

// ---------------------------------------------------------------------------
// Excel 行列上限（与 OOXML 规范一致）
// ---------------------------------------------------------------------------

inline constexpr int kExcelMaxRowCount = 1048576;   // 2^20
inline constexpr int kExcelMaxColCount = 16384;     // 2^14 = XFD
inline constexpr int kExcelMaxColIndex = kExcelMaxColCount - 1;
inline constexpr int kExcelMaxRowIndex = kExcelMaxRowCount - 1;

// ---------------------------------------------------------------------------
// PowerPoint
// ---------------------------------------------------------------------------

inline constexpr int kPptxSlideWidthEmu  = 9144000;  // 10 inch (4:3 default)
inline constexpr int kPptxSlideHeightEmu = 6858000;  // 7.5 inch (4:3 default)
inline constexpr int kPptxWideSlideWidthEmu  = 12192000; // 13.333 inch (16:9)
inline constexpr int kPptxWideSlideHeightEmu = 6858000;  // 7.5 inch (16:9)

} // namespace constants
} // namespace office
} // namespace zq
