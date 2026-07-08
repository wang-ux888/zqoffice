// =============================================================================
// src/ooxml/pml/pml_writer.h
//
// PmlWriter：PresentationML XML 写入器
//
//   生成 OOXML PowerPoint 所需的 XML 部件：
//     - [Content_Types].xml
//     - _rels/.rels
//     - ppt/presentation.xml
//     - ppt/_rels/presentation.xml.rels
//     - ppt/slides/slideN.xml
//     - ppt/slideLayouts/slideLayout1.xml（最小化）
//     - ppt/slideMasters/slideMaster1.xml（最小化）
//     - ppt/_rels/slideMasters/slideMaster1.xml.rels
//     - ppt/theme/theme1.xml（最小化）
//
//   所有方法均为静态：给定输入数据结构 → 返回 XML 字符串
//   生成的 XML 严格遵循 ECMA-376 规范，可被 PowerPoint/ZQ/WPS 打开
//
// 与 PmlReader 对称：
//   PmlReader: XML 字符串 → 结构化数据（GroupShapeNode 树）
//   PmlWriter: 结构化数据 → XML 字符串
//
// JSON schema（ZQOffice 自有）见 17-phase-h-detailed-plan.md §3.3
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {
namespace pml {

// ---------------------------------------------------------------------------
// 输入数据结构（写入用）
// ---------------------------------------------------------------------------

/// 几何信息（EMU 单位）
struct WriteGeometry {
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t width = 0;
    std::int64_t height = 0;
};

/// 字体信息
struct WriteFont {
    std::string family;        ///< 字体族（如 "Calibri"）
    std::int32_t size = 0;     ///< 字号（0 表示未设置，单位：1/100 pt，与 OOXML 一致）
    bool bold = false;
    bool italic = false;
    bool underline = false;
    std::string color;         ///< RRGGBB（空表示默认）
};

/// 文本 run
struct WriteRun {
    std::string text;
    WriteFont font;
};

/// 段落对齐方式
enum class WriteAlignment {
    Left,
    Center,
    Right,
    Justify,
};

/// 段落
struct WriteParagraph {
    WriteAlignment alignment = WriteAlignment::Left;
    std::vector<WriteRun> runs;
};

/// 形状类型
enum class WriteShapeType {
    Text,       ///< 文本框
    Image,      ///< 图片
    Chart,      ///< 图表
    Group,      ///< 组合形状
    AutoShape,  ///< 自选图形（prstShape）
};

/// 形状（写入用）
struct WriteShape {
    std::string id;                  ///< 形状 ID（如 "shape_0"）
    std::string name;                ///< 形状名称
    WriteShapeType type = WriteShapeType::Text;
    WriteGeometry geometry;
    std::int32_t rotation = 0;       ///< 旋转角度（1/60000 度，OOXML 约定）

    /// 文本内容（type == Text/AutoShape 时有效）
    std::vector<WriteParagraph> paragraphs;

    /// 填充颜色（RRGGBB，空表示无填充）
    std::string fillColor;

    /// 图片路径（type == Image 时有效，如 "media/image1.png"）
    std::string imagePath;

    /// 图表类型（type == Chart 时有效，如 "bar"/"line"/"pie"）
    std::string chartType;
    std::string chartTitle;
};

/// 备注（notesSlide）
struct WriteNotes {
    std::string text;
};

/// 幻灯片（写入用）
struct WriteSlide {
    std::vector<WriteShape> shapes;
    WriteNotes notes;
};

/// 幻灯片尺寸（EMU）
struct WriteSlideSize {
    std::int64_t width = 9144000;    ///< 默认 10 inch（4:3）
    std::int64_t height = 6858000;   ///< 默认 7.5 inch
};

/// 核心属性（与 sml::WriteCoreProperties 对齐，但独立定义避免跨模块依赖）
struct WriteCoreProperties {
    std::string title;
    std::string creator;
    std::string created;    ///< ISO 8601 格式
    std::string modified;   ///< ISO 8601 格式
};

// ---------------------------------------------------------------------------
// PmlWriter：所有方法均为静态
// ---------------------------------------------------------------------------
class PmlWriter {
public:
    // -----------------------------------------------------------------------
    // XML 部件生成
    // -----------------------------------------------------------------------

    /// 生成 [Content_Types].xml
    /// @param slideCount  幻灯片数量
    /// @param hasCoreProps  是否包含 docProps/core.xml
    static std::string writeContentTypes(std::size_t slideCount,
                                          bool hasCoreProps);

    /// 生成 _rels/.rels（根关系）
    static std::string writeRootRels();

    /// 生成 ppt/presentation.xml
    /// @param slideCount  幻灯片数量
    /// @param slideSize   幻灯片尺寸
    static std::string writePresentation(std::size_t slideCount,
                                          const WriteSlideSize& slideSize);

    /// 生成 ppt/_rels/presentation.xml.rels
    /// @param slideCount  幻灯片数量（生成 rId1..rIdN 指向 slides/slideN.xml）
    static std::string writePresentationRels(std::size_t slideCount);

    /// 生成 ppt/slides/slideN.xml
    static std::string writeSlide(const WriteSlide& slide);

    /// 生成 ppt/slideLayouts/slideLayout1.xml（最小化空白版式）
    static std::string writeSlideLayout();

    /// 生成 ppt/slideMasters/slideMaster1.xml（最小化母版）
    static std::string writeSlideMaster();

    /// 生成 ppt/_rels/slideMasters/slideMaster1.xml.rels
    static std::string writeSlideMasterRels();

    /// 生成 ppt/theme/theme1.xml（最小化主题）
    static std::string writeTheme();

    /// 生成 docProps/core.xml
    static std::string writeCoreProperties(const WriteCoreProperties& props);

    // -----------------------------------------------------------------------
    // 工具函数
    // -----------------------------------------------------------------------

    /// XML 转义（& < > " '）
    static std::string escapeXml(const std::string& s);

    /// 段落对齐 → OOXML algn 属性值
    static const char* alignmentAttr(WriteAlignment a);

    /// 形状类型 → JSON type 字符串（与读取管线对齐）
    static const char* shapeTypeName(WriteShapeType t);
};

} // namespace pml
} // namespace ooxml
} // namespace office
} // namespace zq
