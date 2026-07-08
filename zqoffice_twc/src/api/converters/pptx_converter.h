// =============================================================================
// src/api/converters/pptx_converter.h
//
// PptxConverter：pptx ↔ ZQ Slide JSON 转换器
//
// Phase H-Part3：实现 PptxToZQSlide（读取）+ ZQSlideToPptx（写入）
//
// 读取管线（H6）：
//   1. ZipReader 打开 pptx（ZIP 容器）
//   2. 解析 ppt/presentation.xml → slide 列表 + slideSize
//   3. 解析 ppt/_rels/presentation.xml.rels → rId → slideN.xml 路径映射
//   4. 对每个 slide：
//      a. PmlReader.parseSlide → GroupShapeNode 树
//      b. 遍历子节点（sp/pic/graphicFrame），提取形状信息
//      c. 解析 notesSlideN.xml（可选）→ 备注
//   5. 组装完整 ZQ Slide JSON（schema 见 17-phase-h-detailed-plan.md §3.3）
//
// 写入管线（H7）：
//   1. 解析 ZQ Slide JSON → 内部文档模型（WriteSlide 数组）
//   2. 生成 XML 部件：
//      - [Content_Types].xml
//      - _rels/.rels
//      - ppt/presentation.xml
//      - ppt/_rels/presentation.xml.rels
//      - ppt/slides/slideN.xml（PmlWriter）
//      - ppt/slideLayouts/slideLayout1.xml
//      - ppt/slideMasters/slideMaster1.xml
//      - ppt/_rels/slideMasters/slideMaster1.xml.rels
//      - ppt/theme/theme1.xml
//      - docProps/core.xml（可选）
//   3. ZipWriter 打包 → 输出 pptx
// =============================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "api/json/json.h"
#include "zq/office/types.h"
#include "ooxml/pml/pml_writer.h"

namespace zq {
namespace office {
namespace ooxml {
class ZipReader;
class XmlNode;
namespace dml {
class GroupShapeNode;
} // namespace dml
namespace pml {
class PmlReader;
} // namespace pml
} // namespace ooxml
} // namespace office
} // namespace zq

namespace zq {
namespace office {
namespace converters {

/// pptx 转换器（pptx ↔ ZQ Slide JSON）
class PptxConverter {
public:
    PptxConverter();
    ~PptxConverter();

    PptxConverter(const PptxConverter&) = delete;
    PptxConverter& operator=(const PptxConverter&) = delete;

    // -----------------------------------------------------------------------
    // 读取：pptx → ZQ Slide JSON
    // -----------------------------------------------------------------------

    /// pptx 文件 → ZQ Slide JSON
    /// @param filePath        pptx 文件路径
    /// @param dataDirectory   数据目录（存放图片等附件，可为空）
    /// @param optionsJson     转换选项 JSON（可传 "{}"）
    /// @return ExportResult JSON 字符串（外层包装：{code, message, data, version}）
    std::string toJson(const std::string& filePath,
                       const std::string& dataDirectory,
                       const std::string& optionsJson);

    // -----------------------------------------------------------------------
    // 写入：ZQ Slide JSON → pptx
    // -----------------------------------------------------------------------

    /// ZQ Slide JSON → pptx 文件
    /// @param clientVarsJson  ZQ Slide 的 clientVars JSON
    /// @param outputPath      输出 pptx 文件路径
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

    /// 打开 pptx zip 并初始化 PmlReader
    bool loadArchive_(const std::string& filePath);

    /// 解析 ppt/presentation.xml → slideIds_ + slideSize_
    bool parsePresentation_();

    /// 解析 ppt/_rels/presentation.xml.rels → rId → slideN.xml 路径映射
    bool parsePresentationRels_();

    /// 解析单个 slide（slideN.xml）→ WriteSlide
    bool parseSlide_(int slideIndex);

    /// 从 GroupShapeNode 提取形状 → JSON
    /// @param node  PmlReader 解析的形状树节点（type=kSlideTree 根或 kGroup）
    /// @param shapesArr  输出的 shapes JSON 数组
    void extractShapes_(const zq::office::ooxml::dml::GroupShapeNode& node,
                          json::JsonValue& shapesArr);

    /// 从单个形状节点提取 JSON
    json::JsonValue buildShapeJson_(const zq::office::ooxml::dml::GroupShapeNode& node);

    /// 从 XmlNode 提取文本（a:txBody）
    json::JsonValue buildTextJson_(const zq::office::ooxml::XmlNode& spNode);

    /// 解析 notesSlideN.xml → 备注文本
    std::string parseNotes_(int slideIndex);

    // -----------------------------------------------------------------------
    // 内部写入辅助
    // -----------------------------------------------------------------------

    /// 解析 ZQ Slide JSON → writeSlides_/writeSlideSize_/writeCore_
    /// @param clientVarsJson  输入 JSON（可能是 ExportResult 包装，也可能是直接 data）
    /// @return 成功返回 true
    bool parseZQSlideJson_(const std::string& clientVarsJson);

    /// 写入 pptx 文件（ZipWriter 打包所有 XML 部件）
    /// @param outputPath  输出文件路径
    /// @return 成功返回 true
    bool writePptx_(const std::string& outputPath);

    /// 构造错误 JSON
    std::string errorJson_(ErrorCode code, const std::string& message);

    // -----------------------------------------------------------------------
    // 内部数据
    // -----------------------------------------------------------------------

    std::unique_ptr<ooxml::ZipReader> zip_;
    std::unique_ptr<ooxml::pml::PmlReader> pmlReader_;

    /// presentation 解析结果
    struct PresentationData {
        /// slide ID 列表（id + rId）
        std::vector<std::pair<std::uint32_t, std::string>> slideIds;
        /// rId → slideN.xml 路径映射
        std::vector<std::pair<std::string, std::string>> rels;
        /// slideSize（EMU）
        std::int64_t slideSizeCx = 9144000;
        std::int64_t slideSizeCy = 6858000;
    } presentation_;

    /// 当前 slide 的解析结果
    struct SlideParseResult {
        std::vector<json::JsonValue> shapes;
        std::string notes;
    };
    std::vector<SlideParseResult> slideResults_;

    /// coreProperties
    struct CoreProperties {
        std::string title;
        std::string creator;
        std::string created;
        std::string modified;
    } coreProps_;

    // -----------------------------------------------------------------------
    // 写入管线数据
    // -----------------------------------------------------------------------

    std::vector<ooxml::pml::WriteSlide> writeSlides_;
    ooxml::pml::WriteSlideSize writeSlideSize_;
    ooxml::pml::WriteCoreProperties writeCore_;

    std::string err_;
};

} // namespace converters
} // namespace office
} // namespace zq
