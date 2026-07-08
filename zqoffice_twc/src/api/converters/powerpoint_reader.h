// =============================================================================
// src/api/converters/powerpoint_reader.h
//
// PowerPointReader：PowerPoint 流式读取器（Phase H-Part5 / H9）
//
// 内部实现 `ttoffice::IReader` 抽象接口的等价能力：
//   - Initialize（打开 ppt/pptx 文件并解析 presentation.xml）
//   - SetOption（设置转换选项）
//   - GetErrorDescription（错误码 → 描述）
//   - 流式增量读取 slide（readNextSlide/seekToSlide/slideCount）
//
// 与 PptxConverter.toJson 的区别：
//   PptxConverter.toJson 一次性解析所有 slides 并返回完整 JSON；
//   PowerPointReader 在 initialize 阶段仅解析 presentation.xml + rels，
//   readNextSlide 按需解析单张 slide，适合大文件流式处理场景。
//
// 当前实现仅支持 .pptx（OOXML ZIP 容器）。.ppt（OLE2 复合文档）格式
// 将在后续 Phase 中接入 CompoundFileParser 后支持。
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "api/json/json.h"
#include "zq/office/types.h"

namespace zq {
namespace office {
namespace ooxml {
class ZipReader;
class XmlNode;
namespace pml {
class PmlReader;
} // namespace pml
namespace dml {
class GroupShapeNode;
} // namespace dml
} // namespace ooxml
} // namespace office
} // namespace zq

namespace zq {
namespace office {
namespace converters {

/// PowerPoint 流式读取器
class PowerPointReader {
public:
    PowerPointReader();
    ~PowerPointReader();

    PowerPointReader(const PowerPointReader&) = delete;
    PowerPointReader& operator=(const PowerPointReader&) = delete;

    // -----------------------------------------------------------------------
    // 生命周期
    // -----------------------------------------------------------------------

    /// 初始化读取器（打开 pptx 文件 + 解析 presentation.xml）
    /// @param filePath  pptx 文件路径
    /// @param err       错误信息输出（失败时填充）
    /// @return 0 表示成功，非 0 为错误码（与 ErrorCode 对齐）
    int initialize(const std::string& filePath, std::string* err);

    // -----------------------------------------------------------------------
    // 流式读取
    // -----------------------------------------------------------------------

    /// 读取下一张 slide 并返回其 JSON 字符串
    /// @return slide 的 JSON 字符串（schema 与 PptxConverter.toJson 中单张 slide 一致）；
    ///         已到末尾返回空字符串
    std::string readNextSlide();

    /// 跳转到指定 slide 索引（0-based）
    /// @param index  目标 slide 索引（0..slideCount()-1）
    /// @return 成功返回 true；index 越界返回 false
    bool seekToSlide(int index);

    /// slide 总数
    int slideCount() const;

    /// 当前 slide 索引（readNextSlide 将读取的下一张）
    /// @return -1 表示尚未开始读取；slideCount() 表示已读完所有 slide
    int currentIndex() const;

    // -----------------------------------------------------------------------
    // 元数据
    // -----------------------------------------------------------------------

    /// slide 尺寸（EMU）
    std::int64_t slideSizeCx() const { return presentation_.slideSizeCx; }
    std::int64_t slideSizeCy() const { return presentation_.slideSizeCy; }

    /// coreProperties（initialize 后可用）
    const std::string& coreTitle() const { return coreProps_.title; }
    const std::string& coreCreator() const { return coreProps_.creator; }
    const std::string& coreCreated() const { return coreProps_.created; }
    const std::string& coreModified() const { return coreProps_.modified; }

    // -----------------------------------------------------------------------
    // 选项与错误
    // -----------------------------------------------------------------------

    /// 设置选项（与 IReader::SetOption 对齐）
    /// @param key    选项键（1=是否提取图片，2=是否解析 notes，等）
    /// @param value  选项值（0=否，1=是）
    void setOption(int key, int value);

    /// 错误码 → 描述（与 IReader::GetErrorDescription 对齐）
    static const char* getErrorDescription(int err);

private:
    // -----------------------------------------------------------------------
    // 内部解析辅助（复用 PptxConverter 的模式）
    // -----------------------------------------------------------------------

    /// 打开 pptx zip
    bool loadArchive_(const std::string& filePath);

    /// 解析 ppt/presentation.xml → slideIds + slideSize
    bool parsePresentation_();

    /// 解析 ppt/_rels/presentation.xml.rels → rId → slideN.xml 路径映射
    bool parsePresentationRels_();

    /// 解析 docProps/core.xml → coreProps_
    bool parseCoreProperties_();

    /// 解析单张 slide（slideN.xml）→ JSON 字符串
    /// @param slideIndex  0-based slide 索引
    /// @param out         输出的 JSON 字符串
    /// @return 成功返回 true
    bool parseSlide_(int slideIndex, std::string* out);

    /// 从 GroupShapeNode 提取 shapes → JSON 数组
    void extractShapes_(const zq::office::ooxml::dml::GroupShapeNode& node,
                          json::JsonValue& shapesArr);

    /// 从单个形状节点构建 JSON
    json::JsonValue buildShapeJson_(const zq::office::ooxml::dml::GroupShapeNode& node);

    /// 从 spNode 提取文本（a:txBody）→ JSON
    json::JsonValue buildTextJson_(const zq::office::ooxml::XmlNode& spNode);

    /// 解析 notesSlideN.xml → 备注文本
    std::string parseNotesSlide_(int slideIndex);

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

    /// coreProperties
    struct CoreProperties {
        std::string title;
        std::string creator;
        std::string created;
        std::string modified;
    } coreProps_;

    /// 当前 slide 索引（-1 表示尚未开始）
    int currentIndex_ = -1;

    /// 选项
    bool extractImagesEnabled_ = false;   // 默认不提取图片
    bool parseNotesEnabled_ = true;       // 默认解析备注

    /// 错误信息
    std::string err_;
};

} // namespace converters
} // namespace office
} // namespace zq
