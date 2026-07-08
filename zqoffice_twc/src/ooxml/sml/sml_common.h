// =============================================================================
// src/ooxml/sml/sml_common.h
//
// SpreadsheetML 公共类型定义
//
//   SpreadsheetML 是 OOXML 中描述 Excel 工作簿的 XML 词汇表，命名空间：
//     s:    http://schemas.openxmlformats.org/spreadsheetml/2006/main
//           （文档中通常无前缀，直接为默认命名空间）
//     r:    http://schemas.openxmlformats.org/officeDocument/2006/relationships
//
//   常用元素：
//     workbook.xml
//       <workbook>           - 工作簿根
//       <sheets>             - 工作表清单
//       <sheet name=".." sheetId=".." r:id=".."/>  - 单个工作表项
//       <definedNames>       - 命名区域
///      <calcPr>             - 计算属性
///
///    worksheetN.xml
///      <worksheet>          - 工作表根
///      <dimension ref="A1:C3"/> - 数据范围
///      <sheetData>          - 数据区
///        <row r="1" spans="1:3"> - 行
///          <c r="A1" s="0" t="s"> - 单元格（t: s=shared string, n=number, b=bool, str=string）
///            <v>0</v>       - 值（共享字符串索引或数字）
///            <is><t>inline</t></is> - 内联字符串
///          </c>
///        </row>
///      <mergeCells>         - 合并单元格
///      <col>                - 列宽
///
///    sharedStrings.xml
///      <sst>                - 共享字符串表根
///        <si>               - 字符串项
///          <t>text</t>      - 纯文本
///          <r>              - 富文本 run
///            <rPr><b/></rPr> - run 属性
///            <t>run text</t>
///          </r>
///        </si>
///
///    styles.xml
///      <styleSheet>
///        <numFmts>          - 自定义数字格式
///        <fonts>            - 字体
///        <fills>            - 填充
///        <borders>          - 边框
///        <cellXfs>          - 单元格格式
///        <cellStyleXfs>     - 单元格样式格式
///      </styleSheet>
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {
namespace sml {

/// 单元格数据类型（c/@t 属性）
enum class CellType {
    kNumber = 0,    // 默认（无 t 属性或 t="n"）
    kSharedString,  // t="s"：值是 sharedStrings 的索引
    kBoolean,       // t="b"
    kInlineString,  // t="inlineStr" / t="str"
    kError,         // t="e"
    kDate,          // 数字但样式为日期（由上层判断）
};

/// 单元格类型名称（与 c/@t 属性值对应）
inline const char* cellTypeName(CellType t) {
    switch (t) {
        case CellType::kNumber:        return "n";
        case CellType::kSharedString:  return "s";
        case CellType::kBoolean:       return "b";
        case CellType::kInlineString:  return "str";
        case CellType::kError:         return "e";
        case CellType::kDate:          return "d";
    }
    return "n";
}

/// 单元格引用（如 "A1"、"B23"、"AA100"）
/// - 列：从字母解析（A=1, B=2, ..., Z=26, AA=27, ...）
/// - 行：从数字解析（1-based）
struct CellRef {
    std::uint32_t col = 0;   // 1-based
    std::uint32_t row = 0;   // 1-based
    bool valid = false;
};

/// 单元格值
struct CellValue {
    CellType type = CellType::kNumber;
    std::string raw;          // 原始值（v 元素文本或 inlineStr 的 t）
    std::int64_t sharedIndex = -1;  // 共享字符串索引（仅 type=kSharedString 有效）
};

/// 工作表项（workbook/sheets/sheet）
struct SheetItem {
    std::string name;        // sheet/@name
    std::uint32_t sheetId = 0;  // sheet/@sheetId
    std::string relId;       // sheet/@r:id（关系到 worksheetN.xml）
};

/// 合并单元格项（mergeCells/mergeCell）
struct MergeRange {
    std::string ref;         // 如 "A1:C3"
};

} // namespace sml
} // namespace ooxml
} // namespace office
} // namespace zq
