// =============================================================================
// src/ooxml/wml/wml_writer.h
//
// WmlWriter：WordprocessingML XML 写入器
//
//   生成 OOXML Word 所需的 XML 部件：
//     - [Content_Types].xml
//     - _rels/.rels
//     - word/document.xml
//     - word/_rels/document.xml.rels
//     - word/styles.xml（最小化）
//     - docProps/core.xml（可选）
//
//   所有方法均为静态：给定输入数据结构 → 返回 XML 字符串
//   生成的 XML 严格遵循 ECMA-376 规范，可被 Word/ZQ/WPS 打开
//
// 与 WmlReader 对称：
//   WmlReader: XML 字符串 → 结构化数据（WmlBlock 数组）
//   WmlWriter: 结构化数据 → XML 字符串
//
// JSON schema（ZQOffice 自有）见 17-phase-h-detailed-plan.md §3.4
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {
namespace wml {

// ---------------------------------------------------------------------------
// 输入数据结构（写入用）
// ---------------------------------------------------------------------------

/// 几何信息（EMU 单位，用于图片尺寸）
struct WriteGeometry {
    std::int64_t width = 0;
    std::int64_t height = 0;
};

/// 字体信息
struct WriteFont {
    std::string family;        ///< 字体族（如 "Calibri"）
    std::int32_t size = 0;     ///< 字号（半磅，OOXML 约定：22 = 11pt；0 表示未设置）
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

/// 表格单元格
struct WriteTableCell {
    std::int32_t row = 0;
    std::int32_t column = 0;
    std::int32_t gridSpan = 1;
    std::string text;
};

/// 表格
struct WriteTable {
    std::int32_t rows = 0;
    std::int32_t columns = 0;
    std::vector<WriteTableCell> cells;
};

/// 图片
struct WriteImage {
    std::string relId;         ///< 关系 ID（如 "rId1"）
    std::int64_t width = 0;    ///< EMU
    std::int64_t height = 0;   ///< EMU
};

/// 块类型
enum class WriteBlockType {
    Paragraph,
    Heading,
    Table,
    Image,
};

/// 文档块
struct WriteBlock {
    WriteBlockType type = WriteBlockType::Paragraph;
    // Paragraph / Heading
    WriteParagraph paragraph;
    std::int32_t headingLevel = 0;  ///< 1-9（仅 Heading 有效）
    // Table
    WriteTable table;
    // Image
    WriteImage image;
};

/// 核心属性
struct WriteCoreProperties {
    std::string title;
    std::string creator;
    std::string created;    ///< ISO 8601
    std::string modified;   ///< ISO 8601
};

// ---------------------------------------------------------------------------
// WmlWriter：所有方法均为静态
// ---------------------------------------------------------------------------

class WmlWriter {
public:
    // -----------------------------------------------------------------------
    // XML 部件生成
    // -----------------------------------------------------------------------

    /// 生成 [Content_Types].xml
    /// @param hasCoreProps  是否包含 docProps/core.xml
    static std::string writeContentTypes(bool hasCoreProps);

    /// 生成 _rels/.rels（根关系）
    static std::string writeRootRels();

    /// 生成 word/document.xml
    /// @param blocks  文档块数组
    static std::string writeDocument(const std::vector<WriteBlock>& blocks);

    /// 生成 word/_rels/document.xml.rels
    /// @param imageCount  图片数量（生成 rId1..rIdN 指向 media/imageN.png）
    static std::string writeDocumentRels(std::size_t imageCount);

    /// 生成 word/styles.xml（最小化：Normal + Heading1-9）
    static std::string writeStyles();

    /// 生成 docProps/core.xml
    static std::string writeCoreProperties(const WriteCoreProperties& props);

    // -----------------------------------------------------------------------
    // 工具函数
    // -----------------------------------------------------------------------

    /// XML 转义（& < > " '）
    static std::string escapeXml(const std::string& s);

    /// 段落对齐 → OOXML w:jc@w:val 值
    static const char* alignmentAttr(WriteAlignment a);

    /// 块类型 → JSON type 字符串（与读取管线对齐）
    static const char* blockTypeName(WriteBlockType t);
};

} // namespace wml
} // namespace ooxml
} // namespace office
} // namespace zq
