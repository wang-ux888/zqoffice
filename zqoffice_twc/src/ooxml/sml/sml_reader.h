// =============================================================================
// src/ooxml/sml/sml_reader.h
//
// SmlReader：SpreadsheetML XML 读取器
//
//   SpreadsheetML 描述 Excel 工作簿：
//     workbook.xml → worksheet1.xml + sharedStrings.xml + styles.xml + ...
//
//   SmlReader 提供：
//     - parseWorkbook：     解析 workbook.xml，提取 sheets/definedNames
//     - parseWorksheet：    解析 worksheetN.xml，提取 dimension/sheetData/mergeCells
//     - parseSharedStrings：解析 sharedStrings.xml，提取字符串表
//     - parseStyles：       解析 styles.xml（仅返回根节点，详情由 Phase D 处理）
//
//   输出：
//     - sheets()：工作表清单（parseWorkbook 后可用）
//     - dimension()：数据范围 ref（parseWorksheet 后可用）
//     - cells()：所有单元格（parseWorksheet 后可用，行/列/类型/值）
//     - merges()：合并单元格范围列表
//     - sharedStrings()：共享字符串表（parseSharedStrings 后可用）
//
//   性能注意：
//     - worksheet 单元格可达百万级，本读取器一次性加载到内存
//     - 富文本（si/r）合并为纯文本（其他格式信息通过 xmlReader() 访问）
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../xml_reader.h"
#include "sml_common.h"

namespace zq {
namespace office {
namespace ooxml {
namespace sml {

/// SpreadsheetML 读取器
class SmlReader {
public:
    SmlReader() = default;
    ~SmlReader() = default;

    SmlReader(const SmlReader&) = delete;
    SmlReader& operator=(const SmlReader&) = delete;

    // -----------------------------------------------------------------------
    // 解析入口
    // -----------------------------------------------------------------------

    /// 解析 workbook.xml
    bool parseWorkbook(const std::string& xml);

    /// 解析 worksheetN.xml
    bool parseWorksheet(const std::string& xml);

    /// 解析 sharedStrings.xml
    bool parseSharedStrings(const std::string& xml);

    /// 解析 styles.xml（仅解析标记，详情由上层处理）
    bool parseStyles(const std::string& xml);

    // -----------------------------------------------------------------------
    // 结果访问
    // -----------------------------------------------------------------------

    /// 工作表清单（parseWorkbook 后可用）
    const std::vector<SheetItem>& sheets() const { return sheets_; }

    /// 数据范围 ref（parseWorksheet 后可用）
    const std::string& dimension() const { return dimension_; }

    /// 所有单元格（parseWorksheet 后可用）
    /// 排序：按行优先（r=1 的所有单元格在前，然后 r=2...）
    const std::vector<std::pair<CellRef, CellValue>>& cells() const { return cells_; }

    /// 合并单元格范围列表（parseWorksheet 后可用）
    const std::vector<MergeRange>& merges() const { return merges_; }

    /// 共享字符串表（parseSharedStrings 后可用）
    const std::vector<std::string>& sharedStrings() const { return sharedStrings_; }

    /// 错误信息
    const std::string& error() const { return error_; }

    /// 是否已解析
    bool isParsed() const { return parsed_; }

    /// 内部 XmlReader（用于访问未结构化的字段）
    const XmlReader& xmlReader() const { return xmlReader_; }

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    /// 解析单元格引用字符串（"A1" → {col=1, row=1}）
    static CellRef parseCellRef(const std::string& ref);

    /// 把列号转为字母（1 → "A", 27 → "AA"）
    static std::string columnToLetters(std::uint32_t col);

    /// 把字母列名转为列号（"A" → 1, "AA" → 27）
    static std::uint32_t lettersToColumn(const std::string& letters);

private:
    // -----------------------------------------------------------------------
    // 内部解析
    // -----------------------------------------------------------------------

    void parseSheets_(const XmlNode& sheetsNode);
    void parseSheetData_(const XmlNode& sheetData);
    void parseMergeCells_(const XmlNode& mergeCells);
    void parseRow_(const XmlNode& row, std::uint32_t rowNum);
    CellValue parseCell_(const XmlNode& cell);

    /// 从 si 节点提取字符串（合并 r 富文本为纯文本）
    std::string extractSharedString_(const XmlNode& si);

    // -----------------------------------------------------------------------
    // 工具
    // -----------------------------------------------------------------------

    static std::string localName_(const std::string& qualified);
    static std::string nodeLocalName_(const XmlNode& node);

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    XmlReader xmlReader_;
    std::vector<SheetItem> sheets_;
    std::string dimension_;
    std::vector<std::pair<CellRef, CellValue>> cells_;
    std::vector<MergeRange> merges_;
    std::vector<std::string> sharedStrings_;
    std::string error_;
    bool parsed_ = false;
};

} // namespace sml
} // namespace ooxml
} // namespace office
} // namespace zq
