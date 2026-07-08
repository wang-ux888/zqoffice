// =============================================================================
// src/drawing/enums/record_type.h
//
// Escher 记录类型枚举（RecordType / EscherRecordType）
//
//   对应 [MS-ODRAW] EscherRecordType（Office Drawing Binary Format）
//   用于 OfficeArt 容器/记录类型识别，描述旧版 Office（.ppt/.xls）中的
//   OfficeArt 二进制流。
//
//   每个记录类型的 verInstance/recType 字段由 Escher 头部决定：
//     - 容器记录（Container）类型值最高位为 0
//     - 数据记录（Atom）类型值最高位为 1
//
//   参考：[MS-ODRAW] section 2.4 RecordType Enumeration
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace zq {
namespace office {
namespace drawing {

/// Escher 记录类型
///
/// 与 [MS-ODRAW] EscherRecordType 对齐
/// 用于 OfficeArt 二进制流解析
enum class RecordType : std::uint16_t {
    kUnknown = 0,

    // --- 容器记录（Container，最高位 = 0）---
    kDggContainer = 0xF000,           // 0xF000 Drawing Group Container
    kDgContainer = 0xF002,            // 0xF002 Drawing Container
    kSpgrContainer = 0xF003,          // 0xF003 Shape Group Container
    kSpContainer = 0xF004,            // 0xF004 Shape Container
    kSolverContainer = 0xF005,        // 0xF005 Solver Container
    kBStoreContainer = 0xF001,        // 0xF001 BStore Container (BLIP store)

    // --- 数据记录（Atom，最高位 = 1）---
    kBlip = 0xF007,                   // 0xF007 BLIP (Binary Large Image)
    kRule = 0xF012,                   // 0xF012 Rule
    kSecondary = 0xF018,              // 0xF018 Secondary
    kClientTextbox = 0xF00D,          // 0xF00D Client Textbox
    kClientAnchor = 0xF010,           // 0xF010 Client Anchor
    kClientData = 0xF011,             // 0xF011 Client Data
    kClientRule = 0xF013,             // 0xF013 Client Rule

    // --- Drawing Group 记录 ---
    kOpt = 0xF00B,                    // 0xF00B Options (property table)
    kOpt2 = 0xF00C,                   // 0xF00C Options 2
    kColorMRU = 0xF01A,               // 0xF01A Color Most Recently Used
    kRegroupItems = 0xF118,           // 0xF118 Regroup Items

    // --- Drawing 记录 ---
    kSpgr = 0xF009,                   // 0xF009 Shape Group
    kSp = 0xF00A,                     // 0xF00A Shape
    kAnchor = 0xF008,                 // 0xF008 Anchor

    // --- Shape 记录（property）---
    kShapeProps = 0xF01F,             // 0xF01F Shape Properties
    kShapeFlagsAtom = 0xF01E,         // 0xF01E Shape Flags Atom

    // --- Solver 记录 ---
    kConnectorRule = 0xF019,          // 0xF019 Connector Rule
    kAlignRule = 0xF01B,             // 0xF01B Align Rule
    kArcRule = 0xF01C,               // 0xF01C Arc Rule
    kCalloutRule = 0xF01D,           // 0xF01D Callout Rule

    // --- BSE 子记录（blipType）---
    kBlipStart = 0xF018,              // 起始
    kBlipEMF = 0xF01A,                // EMF
    kBlipWMF = 0xF01B,                // WMF
    kBlipPICT = 0xF01C,               // PICT
    kBlipJPEG = 0xF01D,               // JPEG
    kBlipPNG = 0xF01E,                // PNG
    kBlipDIB = 0xF01F,                // DIB
    kBlipTIFF = 0xF020,               // TIFF

    // --- OfficeArt 客户端扩展 ---
    kOfficeArtClientTextbox = 0xF00D, // Excel/PPT 文本框客户端
    kOfficeArtClientAnchor = 0xF010,  // Excel/PPT 锚点客户端
    kOfficeArtClientData = 0xF011,    // Excel/PPT 数据客户端
    kOfficeArtClientRule = 0xF013,    // Excel/PPT 规则客户端
};

/// 枚举 → 16 进制字符串（如 "0xF000"）
const char* recordTypeToString(RecordType t);

/// 字符串 → 枚举
/// 支持十进制和 16 进制（如 "0xF000" 或 "61440"）
RecordType recordTypeFromString(std::string_view s);

/// 是否为容器记录（Container）
bool isContainerRecord(RecordType t);

} // namespace drawing
} // namespace office
} // namespace zq
