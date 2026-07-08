// =============================================================================
// src/api/converters/docx_converter.h
//
// DocxConverter：docx ↔ ZQ Doc JSON 转换器
//
// Phase H-Part4：实现 DocxToZQDoc（读取）+ ZQDocToDocx（写入）
//
// 读取管线（H8-a）：
//   1. ZipReader 打开 docx（ZIP 容器）
//   2. 解析 word/document.xml（WmlReader）→ blocks 数组
//   3. 解析 word/styles.xml（WmlReader）→ 样式名表（可选）
//   4. 解析 word/_rels/document.xml.rels → rId → media 路径映射
//   5. 组装 ZQ Doc JSON（blocks + coreProperties）
//
// 写入管线（H8-b）：
//   1. 解析 ZQ Doc JSON → 内部文档模型（WriteBlock 数组）
//   2. 生成 XML 部件：
//      - [Content_Types].xml
//      - _rels/.rels
//      - word/document.xml（WmlWriter）
//      - word/_rels/document.xml.rels
//      - word/styles.xml（最小化）
//      - docProps/core.xml（可选）
//   3. ZipWriter 打包 → 输出 docx
// =============================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "api/json/json.h"
#include "zq/office/types.h"
#include "ooxml/wml/wml_reader.h"
#include "ooxml/wml/wml_writer.h"

namespace zq {
namespace office {
namespace ooxml {
class ZipReader;
} // namespace ooxml
} // namespace office
} // namespace zq

namespace zq {
namespace office {
namespace converters {

/// docx 转换器（docx ↔ ZQ Doc JSON）
class DocxConverter {
public:
    DocxConverter();
    ~DocxConverter();

    DocxConverter(const DocxConverter&) = delete;
    DocxConverter& operator=(const DocxConverter&) = delete;

    // -----------------------------------------------------------------------
    // 读取：docx → ZQ Doc JSON
    // -----------------------------------------------------------------------

    /// docx 文件 → ZQ Doc JSON
    /// @param filePath        docx 文件路径
    /// @param dataDirectory   数据目录（存放图片等附件，可为空）
    /// @param optionsJson     转换选项 JSON（可传 "{}"）
    /// @return ExportResult JSON 字符串
    std::string toJson(const std::string& filePath,
                       const std::string& dataDirectory,
                       const std::string& optionsJson);

    // -----------------------------------------------------------------------
    // 写入：ZQ Doc JSON → docx
    // -----------------------------------------------------------------------

    /// ZQ Doc JSON → docx 文件
    /// @param clientVarsJson  ZQ Doc 的 clientVars JSON
    /// @param outputPath      输出 docx 文件路径
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

    /// 打开 docx zip 并初始化
    bool loadArchive_(const std::string& filePath);

    /// 解析 word/document.xml → blocks_
    bool parseDocument_();

    /// 解析 word/styles.xml → styleNames_（可选）
    bool parseStyles_();

    /// 解析 word/_rels/document.xml.rels → rId → media 路径映射
    bool parseDocumentRels_();

    /// 解析 docProps/core.xml → coreProps_
    bool parseCoreProperties_();

    /// 将 blocks_ 转换为 JSON 数组
    json::JsonValue buildBlocksJson_();

    // -----------------------------------------------------------------------
    // 内部写入辅助
    // -----------------------------------------------------------------------

    /// 解析 ZQ Doc JSON → writeBlocks_/writeCore_
    /// @param clientVarsJson  输入 JSON（可能是 ExportResult 包装，也可能是直接 data）
    /// @return 成功返回 true
    bool parseZQDocJson_(const std::string& clientVarsJson);

    /// 写入 docx 文件（ZipWriter 打包所有 XML 部件）
    bool writeDocx_(const std::string& outputPath);

    /// 构造错误 JSON
    std::string errorJson_(ErrorCode code, const std::string& message);

    // -----------------------------------------------------------------------
    // 内部数据
    // -----------------------------------------------------------------------

    std::unique_ptr<ooxml::ZipReader> zip_;

    /// 读取管线数据
    struct ReadData {
        std::vector<ooxml::wml::WmlBlock> blocks;
        std::vector<std::string> styleNames;
        /// rId → media 路径
        std::vector<std::pair<std::string, std::string>> rels;
        /// coreProperties
        std::string title;
        std::string creator;
        std::string created;
        std::string modified;
    } read_;

    /// 写入管线数据
    std::vector<ooxml::wml::WriteBlock> writeBlocks_;
    ooxml::wml::WriteCoreProperties writeCore_;

    std::string err_;
};

} // namespace converters
} // namespace office
} // namespace zq
