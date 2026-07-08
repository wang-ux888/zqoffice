// =============================================================================
// src/ooxml/sml/sml_writer.h
//
// SmlWriter：SpreadsheetML XML 写入器
//
//   生成 OOXML Excel 所需的 XML 部件：
//     - [Content_Types].xml
//     - _rels/.rels
//     - xl/workbook.xml
//     - xl/_rels/workbook.xml.rels
//     - xl/worksheets/sheetN.xml
//     - xl/sharedStrings.xml
//     - xl/styles.xml
//     - docProps/core.xml
//
//   所有方法均为静态：给定输入数据结构 → 返回 XML 字符串
//   生成的 XML 严格遵循 ECMA-376 规范，可被 Excel/ZQ/WPS 打开
//
// 与 SmlReader 对称：
//   SmlReader: XML 字符串 → 结构化数据
//   SmlWriter: 结构化数据 → XML 字符串
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {
namespace sml {

// ---------------------------------------------------------------------------
// 输入数据结构（与 SmlReader 的输出结构对齐）
// ---------------------------------------------------------------------------

/// 工作表信息（workbook/sheets/sheet）
struct WriteSheetInfo {
    std::string name;          ///< sheet 名称
    std::uint32_t sheetId = 0;  ///< sheetId（1-based）
    std::string relId;          ///< r:id（如 "rId1"）
};

/// 定义名称（workbook/definedNames/definedName）
struct WriteDefinedName {
    std::string name;          ///< 名称
    std::string ref;           ///< 引用（如 "Sheet1!$A$1:$B$2"）
};

/// 单元格值类型
enum class WriteCellType {
    Number,        ///< 数字（默认）
    SharedString,  ///< 共享字符串（值为 sharedStrings 索引）
    Boolean,       ///< 布尔
    InlineString,  ///< 内联字符串
    Error,         ///< 错误
};

/// 单元格（写入用）
struct WriteCell {
    std::uint32_t row = 0;   ///< 1-based 行号
    std::uint32_t col = 0;   ///< 1-based 列号
    WriteCellType type = WriteCellType::Number;
    std::string value;        ///< 值（数字为字符串形式，字符串为内容或索引）
};

/// 行（写入用）
struct WriteRow {
    std::uint32_t row = 0;   ///< 1-based 行号
    std::vector<WriteCell> cells;
};

/// 合并单元格范围
struct WriteMergeRange {
    std::uint32_t startRow = 0;  ///< 1-based
    std::uint32_t startCol = 0;  ///< 1-based
    std::uint32_t endRow = 0;
    std::uint32_t endCol = 0;
};

/// 工作表数据（写入用）
struct WriteSheetData {
    std::vector<WriteRow> rows;
    std::vector<WriteMergeRange> merges;
};

/// 核心属性（docProps/core.xml）
struct WriteCoreProperties {
    std::string title;
    std::string creator;
    std::string created;    ///< ISO 8601 格式
    std::string modified;   ///< ISO 8601 格式
};

// ---------------------------------------------------------------------------
// SmlWriter：所有方法均为静态
// ---------------------------------------------------------------------------
class SmlWriter {
public:
    // -----------------------------------------------------------------------
    // XML 部件生成
    // -----------------------------------------------------------------------

    /// 生成 [Content_Types].xml
    /// @param sheetCount  工作表数量（用于生成 Override 条目）
    /// @param hasSharedStrings  是否包含 sharedStrings.xml
    static std::string writeContentTypes(std::size_t sheetCount,
                                          bool hasSharedStrings);

    /// 生成 _rels/.rels（根关系）
    /// @param hasCoreProps  是否包含 docProps/core.xml
    static std::string writeRootRels(bool hasCoreProps);

    /// 生成 xl/workbook.xml
    static std::string writeWorkbook(const std::vector<WriteSheetInfo>& sheets,
                                      const std::vector<WriteDefinedName>& names);

    /// 生成 xl/_rels/workbook.xml.rels
    /// @param sheetCount  工作表数量（生成 rId1..rIdN）
    /// @param hasSharedStrings
    /// @param hasStyles
    static std::string writeWorkbookRels(std::size_t sheetCount,
                                          bool hasSharedStrings,
                                          bool hasStyles);

    /// 生成 xl/worksheets/sheetN.xml
    static std::string writeWorksheet(const WriteSheetData& data);

    /// 生成 xl/sharedStrings.xml
    static std::string writeSharedStrings(const std::vector<std::string>& strings);

    /// 生成 xl/styles.xml（最小化：仅默认样式）
    static std::string writeStyles();

    /// 生成 docProps/core.xml
    static std::string writeCoreProperties(const WriteCoreProperties& props);

    // -----------------------------------------------------------------------
    // 工具函数
    // -----------------------------------------------------------------------

    /// XML 转义（& < > " '）
    static std::string escapeXml(const std::string& s);

    /// 列号 → 字母（1 → "A", 27 → "AA"）
    static std::string columnToLetters(std::uint32_t col);

    /// 单元格引用（row + col → "A1"）
    static std::string cellRef(std::uint32_t row, std::uint32_t col);

    /// 单元格类型 → c/@t 属性值（Number 返回空字符串，表示默认）
    static const char* cellTypeAttr(WriteCellType t);
};

} // namespace sml
} // namespace ooxml
} // namespace office
} // namespace zq
