// =============================================================================
// src/api/converters/xlsx_converter.h
//
// XlsxConverter：xlsx ↔ ZQ Sheet JSON 转换器
//
// Phase H4：实现 XlsxToZQSheet（xlsx → JSON 读取管线）
// Phase H5：实现 ZQSheetToXlsx（JSON → xlsx 写入管线，待实施）
//
// 读取管线（H4）：
//   1. ZipReader 打开 xlsx（ZIP 容器）
//   2. 解析 xl/workbook.xml → sheets 清单 + definedNames
//   3. 解析 xl/sharedStrings.xml → 字符串表（可选）
//   4. 解析 xl/styles.xml → 数字格式索引（用于日期检测）
//   5. 对每个 sheet：
//      a. 通过 xl/_rels/workbook.xml.rels 找到 worksheetN.xml
//      b. SmlReader.parseWorksheet → cells + merges + dimension
//      c. 遍历 cells，构建 cell JSON（类型解析 + 日期检测）
//   6. 组装完整 ZQ Sheet JSON
//
// JSON schema（ZQOffice 自有）见 17-phase-h-detailed-plan.md §3.2
// =============================================================================
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "api/json/json.h"
#include "zq/office/types.h"
#include "ooxml/sml/sml_writer.h"

namespace zq {
namespace office {
namespace ooxml {
class ZipReader;
namespace sml {
class SmlReader;
struct SheetItem;
struct CellRef;
struct CellValue;
struct MergeRange;
} // namespace sml
} // namespace ooxml
} // namespace office
} // namespace zq

namespace zq {
namespace office {
namespace converters {

/// xlsx 转换器（xlsx ↔ ZQ Sheet JSON）
class XlsxConverter {
public:
    XlsxConverter();
    ~XlsxConverter();

    XlsxConverter(const XlsxConverter&) = delete;
    XlsxConverter& operator=(const XlsxConverter&) = delete;

    // -----------------------------------------------------------------------
    // 读取：xlsx → ZQ Sheet JSON
    // -----------------------------------------------------------------------

    /// xlsx 文件 → ZQ Sheet JSON
    /// @param filePath        xlsx 文件路径
    /// @param dataDirectory   数据目录（存放图片等附件，可为空）
    /// @param optionsJson     转换选项 JSON（可传 "{}"）
    /// @return ExportResult JSON 字符串（外层包装：{code, message, data, version}）
    std::string toJson(const std::string& filePath,
                       const std::string& dataDirectory,
                       const std::string& optionsJson);

    // -----------------------------------------------------------------------
    // 写入：ZQ Sheet JSON → xlsx（Phase H5 实施）
    // -----------------------------------------------------------------------

    /// ZQ Sheet JSON → xlsx 文件
    /// @param clientVarsJson  ZQ Sheet 的 clientVars JSON
    /// @param outputPath      输出 xlsx 文件路径
    /// @param dataDirectory   数据目录
    /// @param optionsJson     转换选项 JSON
    /// @return ExportResult JSON 字符串
    std::string fromJson(const std::string& clientVarsJson,
                         const std::string& outputPath,
                         const std::string& dataDirectory,
                         const std::string& optionsJson);

private:
    // -----------------------------------------------------------------------
    // 内部读取辅助
    // -----------------------------------------------------------------------

    /// 打开 xlsx zip 并解析 workbook.xml + sharedStrings + styles
    /// @return 成功返回 true，失败设置 err_
    bool loadArchive_(const std::string& filePath);

    // -----------------------------------------------------------------------
    // 内部写入辅助（Phase H5）
    // -----------------------------------------------------------------------

    /// 解析 ZQ Sheet JSON → 内部文档模型（writeSheets_/writeNames_/writeCore_）
    /// @param clientVarsJson  输入 JSON（可能是 ExportResult 包装，也可能是直接 data）
    /// @return 成功返回 true
    bool parseZQSheetJson_(const std::string& clientVarsJson);

    /// 收集所有字符串单元格 → sharedStrings 表 + 索引映射
    void collectSharedStrings_();

    /// 写入 xlsx 文件（ZipWriter 打包所有 XML 部件）
    /// @param outputPath  输出文件路径
    /// @return 成功返回 true
    bool writeXlsx_(const std::string& outputPath);

    /// 把一个 sheet 的 JSON → WriteSheetData（含 rows + merges）
    /// @param sheetJson  sheet 的 JSON 对象
    /// @param out        输出的 WriteSheetData
    /// @return 成功返回 true
    bool buildWriteSheetData_(const json::JsonValue& sheetJson,
                               ooxml::sml::WriteSheetData& out);

    /// 解析 workbook.xml + workbook.xml.rels
    bool parseWorkbook_();

    /// 解析 sharedStrings.xml（如果存在）
    bool parseSharedStrings_();

    /// 解析 styles.xml 中的 numFmts（用于日期检测）
    bool parseStyles_();

    /// 解析单个工作表（worksheetN.xml）
    bool parseWorksheet_(int sheetIndex);

    /// 构建 sheet JSON
    json::JsonValue buildSheetJson_(int sheetIndex);

    /// 构建单元格 JSON
    json::JsonValue buildCellJson_(const ooxml::sml::CellRef& ref,
                                   const ooxml::sml::CellValue& value);

    /// 构建 definedNames JSON
    json::JsonValue buildDefinedNamesJson_();

    /// 构建 coreProperties JSON
    json::JsonValue buildCorePropertiesJson_();

    /// 判断 numFmt 索引 + 格式串是否为日期
    bool isDateFormat_(int numFmtId, const std::string& formatCode) const;

    /// 构造错误 JSON
    std::string errorJson_(ErrorCode code, const std::string& message);

    // -----------------------------------------------------------------------
    // 内部数据
    // -----------------------------------------------------------------------

    std::unique_ptr<ooxml::ZipReader> zip_;
    std::unique_ptr<ooxml::sml::SmlReader> smlReader_;

    /// workbook 解析结果
    struct WorkbookData {
        std::vector<ooxml::sml::SheetItem> sheets;
        /// r:id → worksheet 文件路径 映射（来自 workbook.xml.rels）
        std::vector<std::pair<std::string, std::string>> rels;
        /// definedNames
        std::vector<std::pair<std::string, std::string>> definedNames;
    } workbook_;

    /// 共享字符串表
    std::vector<std::string> sharedStrings_;

    /// 数字格式表（numFmtId → formatCode）
    std::vector<std::pair<int, std::string>> numFmts_;

    /// 当前工作表的解析结果索引（用于 buildSheetJson_）
    struct SheetParseResult {
        std::string dimension;
        std::vector<std::pair<ooxml::sml::CellRef, ooxml::sml::CellValue>> cells;
        std::vector<ooxml::sml::MergeRange> merges;
    };
    std::vector<SheetParseResult> sheetResults_;

    /// coreProperties
    struct CoreProperties {
        std::string title;
        std::string creator;
        std::string created;
        std::string modified;
    } coreProps_;

    // -----------------------------------------------------------------------
    // 写入管线数据（Phase H5）
    // -----------------------------------------------------------------------

    /// 写入用：sheet 描述（从 JSON 解析得到）
    struct WriteSheetMeta {
        std::string name;
        std::string sheetId;       ///< JSON 中的 sheetId 字段（如 "sheet_0"）
        ooxml::sml::WriteSheetData data;
    };
    std::vector<WriteSheetMeta> writeSheets_;

    /// 写入用：definedNames
    std::vector<ooxml::sml::WriteDefinedName> writeNames_;

    /// 写入用：coreProperties
    ooxml::sml::WriteCoreProperties writeCore_;

    /// 写入用：sharedStrings 表（去重后）
    std::vector<std::string> writeSharedStrings_;

    /// 字符串 → sharedStrings 索引映射（去重用）
    std::unordered_map<std::string, std::size_t> writeStringIndex_;

    std::string err_;
};

} // namespace converters
} // namespace office
} // namespace zq
