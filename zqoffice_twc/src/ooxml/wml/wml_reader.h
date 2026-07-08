// =============================================================================
// src/ooxml/wml/wml_reader.h
//
// WmlReader：WordprocessingML XML 读取器
//
//   WordprocessingML 描述 Word 文档的结构：
//     word/document.xml → w:document/w:body → 段落(w:p) / 表格(w:tbl)
//
//   WmlReader 提供：
//     - parseDocument：解析 w:document（提取 body 中的 blocks）
//     - parseStyles：  解析 word/styles.xml（提取样式名表，可选）
//
//   输出：
//     - blocks()：返回 WmlBlock 数组（Paragraph / Table / Image）
//     - 段落包含 runs（文本 + 字体属性）
//     - 表格包含 cells（行/列/文本）
//     - 图片包含 relId（r:embed）+ 尺寸（EMU）
//
//   注意：图片关系（rId → media path）由 DocxConverter 通过
//         word/_rels/document.xml.rels 解析后回填
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../xml_reader.h"

namespace zq {
namespace office {
namespace ooxml {
namespace wml {

// ---------------------------------------------------------------------------
// 数据结构
// ---------------------------------------------------------------------------

/// 字体属性（与 OOXML w:rPr 对齐）
struct WmlFont {
    std::string family;        ///< 字体族（w:rFonts@w:ascii）
    std::int32_t size = 0;     ///< 字号（半磅，OOXML 约定：22 = 11pt）
    bool bold = false;         ///< w:b
    bool italic = false;       ///< w:i
    bool underline = false;    ///< w:u
    std::string color;         ///< RRGGBB（w:color@w:val）
};

/// 文本 run
struct WmlRun {
    std::string text;          ///< w:t 内容
    WmlFont font;
};

/// 段落
struct WmlParagraph {
    std::string style;         ///< w:pStyle@w:val（如 "Normal" / "Heading1"）
    std::string alignment;     ///< w:jc@w:val（"left"/"center"/"right"/"both"）
    std::vector<WmlRun> runs;
};

/// 表格单元格
struct WmlTableCell {
    std::int32_t row = 0;
    std::int32_t column = 0;
    std::int32_t gridSpan = 1;
    std::string text;          ///< 简化：单元格内所有 run 文本拼接
};

/// 表格
struct WmlTable {
    std::int32_t rows = 0;
    std::int32_t columns = 0;
    std::vector<WmlTableCell> cells;
};

/// 图片
struct WmlImage {
    std::string relId;         ///< a:blip@r:embed
    std::int64_t width = 0;    ///< EMU（a:ext@cx）
    std::int64_t height = 0;   ///< EMU（a:ext@cy）
};

/// 块类型
enum class WmlBlockType {
    Paragraph,
    Table,
    Image,
};

/// 文档块（body 的直接子节点）
struct WmlBlock {
    WmlBlockType type = WmlBlockType::Paragraph;
    WmlParagraph paragraph;
    WmlTable table;
    WmlImage image;
};

// ---------------------------------------------------------------------------
// WmlReader
// ---------------------------------------------------------------------------

/// WordprocessingML 读取器
class WmlReader {
public:
    WmlReader() = default;
    ~WmlReader() = default;

    WmlReader(const WmlReader&) = delete;
    WmlReader& operator=(const WmlReader&) = delete;

    // -----------------------------------------------------------------------
    // 解析入口
    // -----------------------------------------------------------------------

    /// 解析 word/document.xml
    /// @param xml  XML 字符串
    /// @return 解析成功返回 true
    bool parseDocument(const std::string& xml);

    /// 解析 word/styles.xml（可选，仅提取样式名表）
    /// @param xml  XML 字符串
    /// @return 解析成功返回 true
    bool parseStyles(const std::string& xml);

    // -----------------------------------------------------------------------
    // 结果访问
    // -----------------------------------------------------------------------

    /// 文档块列表（body 子节点）
    const std::vector<WmlBlock>& blocks() const { return blocks_; }

    /// 已知样式名列表（来自 styles.xml）
    const std::vector<std::string>& styleNames() const { return styleNames_; }

    /// 错误信息
    const std::string& error() const { return error_; }

    /// 是否已解析
    bool isParsed() const { return parsed_; }

private:
    // -----------------------------------------------------------------------
    // 内部解析
    // -----------------------------------------------------------------------

    /// 解析 w:body 子节点
    void parseBody_(const XmlNode& body);

    /// 解析 w:p（段落）→ WmlParagraph
    WmlParagraph parseParagraph_(const XmlNode& pNode);

    /// 解析 w:r（run）→ WmlRun
    WmlRun parseRun_(const XmlNode& rNode);

    /// 解析 w:rPr（run 属性）→ WmlFont
    WmlFont parseRPr_(const XmlNode& rPrNode);

    /// 解析 w:tbl（表格）→ WmlTable
    WmlTable parseTable_(const XmlNode& tblNode);

    /// 解析 w:drawing（图片）→ WmlImage
    /// @param drawingNode  w:drawing 节点
    /// @param outImage     输出的图片信息
    /// @return 找到图片返回 true
    bool parseDrawing_(const XmlNode& drawingNode, WmlImage& outImage);

    // -----------------------------------------------------------------------
    // 工具
    // -----------------------------------------------------------------------

    static std::string localName_(const std::string& qualified);
    static std::string nodeLocalName_(const XmlNode& node);

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    XmlReader xmlReader_;
    std::vector<WmlBlock> blocks_;
    std::vector<std::string> styleNames_;
    std::string error_;
    bool parsed_ = false;
};

} // namespace wml
} // namespace ooxml
} // namespace office
} // namespace zq
