// =============================================================================
// src/ooxml/wml/wml_writer.cpp
//
// WmlWriter 实现：生成 WordprocessingML XML 部件
// =============================================================================
#include "wml_writer.h"

#include <cstdio>
#include <sstream>
#include <string>

namespace zq {
namespace office {
namespace ooxml {
namespace wml {

// ===========================================================================
// 工具函数
// ===========================================================================

std::string WmlWriter::escapeXml(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

const char* WmlWriter::alignmentAttr(WriteAlignment a) {
    switch (a) {
        case WriteAlignment::Left:    return "left";
        case WriteAlignment::Center:  return "center";
        case WriteAlignment::Right:   return "right";
        case WriteAlignment::Justify: return "both";
    }
    return "left";
}

const char* WmlWriter::blockTypeName(WriteBlockType t) {
    switch (t) {
        case WriteBlockType::Paragraph: return "paragraph";
        case WriteBlockType::Heading:   return "heading";
        case WriteBlockType::Table:     return "table";
        case WriteBlockType::Image:     return "image";
    }
    return "paragraph";
}

// ===========================================================================
// 内部辅助：生成 w:rPr
// ===========================================================================

namespace {

/// 生成 w:rPr（run 属性）XML
std::string writeRPr_(const WriteFont& font) {
    std::string xml;
    bool hasAny = !font.family.empty() || font.size > 0 ||
                  font.bold || font.italic || font.underline ||
                  !font.color.empty();
    if (!hasAny) return xml;

    xml += "<w:rPr>";
    if (!font.family.empty()) {
        xml += "<w:rFonts w:ascii=\"";
        xml += WmlWriter::escapeXml(font.family);
        xml += "\" w:hAnsi=\"";
        xml += WmlWriter::escapeXml(font.family);
        xml += "\"/>";
    }
    if (font.bold)      xml += "<w:b/>";
    if (font.italic)    xml += "<w:i/>";
    if (font.underline) xml += "<w:u w:val=\"single\"/>";
    if (font.size > 0) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "<w:sz w:val=\"%d\"/>",
                      static_cast<int>(font.size));
        xml += buf;
    }
    if (!font.color.empty()) {
        xml += "<w:color w:val=\"";
        xml += WmlWriter::escapeXml(font.color);
        xml += "\"/>";
    }
    xml += "</w:rPr>";
    return xml;
}

/// 生成单个 w:p（段落）
std::string writeParagraph_(const WriteParagraph& para,
                              const std::string& pStyleVal = "") {
    std::string xml;
    xml += "<w:p>";

    // w:pPr
    bool hasPPr = !pStyleVal.empty() ||
                  para.alignment != WriteAlignment::Left;
    if (hasPPr) {
        xml += "<w:pPr>";
        if (!pStyleVal.empty()) {
            xml += "<w:pStyle w:val=\"";
            xml += WmlWriter::escapeXml(pStyleVal);
            xml += "\"/>";
        }
        if (para.alignment != WriteAlignment::Left) {
            xml += "<w:jc w:val=\"";
            xml += WmlWriter::alignmentAttr(para.alignment);
            xml += "\"/>";
        }
        xml += "</w:pPr>";
    }

    // w:r（runs）
    for (const auto& run : para.runs) {
        xml += "<w:r>";
        xml += writeRPr_(run.font);
        xml += "<w:t xml:space=\"preserve\">";
        xml += WmlWriter::escapeXml(run.text);
        xml += "</w:t>";
        xml += "</w:r>";
    }

    xml += "</w:p>";
    return xml;
}

} // namespace

// ===========================================================================
// XML 部件生成
// ===========================================================================

// [Content_Types].xml
std::string WmlWriter::writeContentTypes(bool hasCoreProps) {
    std::string xml;
    xml.reserve(1024);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">";
    xml += "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>";
    xml += "<Default Extension=\"xml\" ContentType=\"application/xml\"/>";
    xml += "<Override PartName=\"/word/document.xml\" "
           "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>";
    xml += "<Override PartName=\"/word/styles.xml\" "
           "ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>";
    if (hasCoreProps) {
        xml += "<Override PartName=\"/docProps/core.xml\" "
               "ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>";
    }
    xml += "</Types>";
    return xml;
}

// _rels/.rels
std::string WmlWriter::writeRootRels() {
    std::string xml;
    xml.reserve(512);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
    xml += "<Relationship Id=\"rId1\" "
           "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
           "Target=\"word/document.xml\"/>";
    xml += "<Relationship Id=\"rId2\" "
           "Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" "
           "Target=\"docProps/core.xml\"/>";
    xml += "</Relationships>";
    return xml;
}

// word/document.xml
std::string WmlWriter::writeDocument(const std::vector<WriteBlock>& blocks) {
    std::string xml;
    xml.reserve(4096);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<w:document "
           "xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
           "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
           "xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\" "
           "xmlns:pic=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">";
    xml += "<w:body>";

    for (const auto& block : blocks) {
        switch (block.type) {
            case WriteBlockType::Paragraph: {
                xml += writeParagraph_(block.paragraph);
                break;
            }
            case WriteBlockType::Heading: {
                // 用 pStyle 引用 HeadingN
                std::string styleVal = "Heading";
                if (block.headingLevel >= 1 && block.headingLevel <= 9) {
                    styleVal += static_cast<char>('0' + block.headingLevel);
                } else {
                    styleVal += "1";
                }
                xml += writeParagraph_(block.paragraph, styleVal);
                break;
            }
            case WriteBlockType::Table: {
                xml += "<w:tbl>";
                // tblPr
                xml += "<w:tblPr>";
                xml += "<w:tblW w:w=\"0\" w:type=\"auto\"/>";
                xml += "<w:tblBorders>";
                xml += "<w:top w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>";
                xml += "<w:left w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>";
                xml += "<w:bottom w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>";
                xml += "<w:right w:val=\"single\" w:sz=\"4\" w:space=\"0\" w:color=\"auto\"/>";
                xml += "</w:tblBorders>";
                xml += "</w:tblPr>";

                // tblGrid（列宽定义）
                xml += "<w:tblGrid>";
                for (std::int32_t c = 0; c < block.table.columns; ++c) {
                    (void)c;
                    xml += "<w:gridCol w:w=\"1000\"/>";
                }
                xml += "</w:tblGrid>";

                // 按 row 分组生成 w:tr
                // 简化：按 cells 的 row 字段分组
                std::int32_t currentRow = -1;
                bool inTr = false;
                for (const auto& cell : block.table.cells) {
                    if (cell.row != currentRow) {
                        if (inTr) xml += "</w:tr>";
                        xml += "<w:tr>";
                        inTr = true;
                        currentRow = cell.row;
                    }
                    // w:tc
                    xml += "<w:tc>";
                    if (cell.gridSpan > 1) {
                        char buf[32];
                        std::snprintf(buf, sizeof(buf),
                                      "<w:tcPr><w:gridSpan w:val=\"%d\"/></w:tcPr>",
                                      static_cast<int>(cell.gridSpan));
                        xml += buf;
                    }
                    // 单元格段落
                    WriteParagraph cellPara;
                    WriteRun run;
                    run.text = cell.text;
                    cellPara.runs.push_back(run);
                    xml += writeParagraph_(cellPara);
                    xml += "</w:tc>";
                }
                if (inTr) xml += "</w:tr>";

                xml += "</w:tbl>";
                // 表格后加空段落（OOXML 要求）
                xml += "<w:p/>";
                break;
            }
            case WriteBlockType::Image: {
                xml += "<w:p>";
                xml += "<w:r>";
                xml += "<w:drawing>";
                xml += "<wp:inline distT=\"0\" distB=\"0\" distL=\"0\" distR=\"0\">";
                xml += "<wp:extent cx=\"";
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%lld",
                              static_cast<long long>(block.image.width));
                xml += buf;
                xml += "\" cy=\"";
                std::snprintf(buf, sizeof(buf), "%lld",
                              static_cast<long long>(block.image.height));
                xml += buf;
                xml += "\"/>";
                xml += "<wp:docPr id=\"1\" name=\"Picture 1\"/>";
                xml += "<a:graphic>";
                xml += "<a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/picture\">";
                xml += "<pic:pic>";
                xml += "<pic:blipFill>";
                xml += "<a:blip r:embed=\"";
                xml += escapeXml(block.image.relId);
                xml += "\"/>";
                xml += "</pic:blipFill>";
                xml += "<pic:spPr>";
                xml += "<a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"";
                std::snprintf(buf, sizeof(buf), "%lld",
                              static_cast<long long>(block.image.width));
                xml += buf;
                xml += "\" cy=\"";
                std::snprintf(buf, sizeof(buf), "%lld",
                              static_cast<long long>(block.image.height));
                xml += buf;
                xml += "\"/></a:xfrm>";
                xml += "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>";
                xml += "</pic:spPr>";
                xml += "</pic:pic>";
                xml += "</a:graphicData>";
                xml += "</a:graphic>";
                xml += "</wp:inline>";
                xml += "</w:drawing>";
                xml += "</w:r>";
                xml += "</w:p>";
                break;
            }
        }
    }

    // sectPr（必需，定义页面大小）
    xml += "<w:sectPr>";
    xml += "<w:pgSz w:w=\"12240\" w:h=\"15840\"/>";  // Letter 8.5x11 inch (twips)
    xml += "<w:pgMar w:top=\"1440\" w:right=\"1440\" w:bottom=\"1440\" w:left=\"1440\"/>";
    xml += "</w:sectPr>";

    xml += "</w:body>";
    xml += "</w:document>";
    return xml;
}

// word/_rels/document.xml.rels
std::string WmlWriter::writeDocumentRels(std::size_t imageCount) {
    std::string xml;
    xml.reserve(512);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
    xml += "<Relationship Id=\"rId1\" "
           "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
           "Target=\"styles.xml\"/>";
    for (std::size_t i = 0; i < imageCount; ++i) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "<Relationship Id=\"rId%zu\" "
                      "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
                      "Target=\"media/image%zu.png\"/>",
                      i + 2, i + 1);
        xml += buf;
    }
    xml += "</Relationships>";
    return xml;
}

// word/styles.xml（最小化）
std::string WmlWriter::writeStyles() {
    std::string xml;
    xml.reserve(2048);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">";
    // Normal
    xml += "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\">";
    xml += "<w:name w:val=\"Normal\"/>";
    xml += "<w:qFormat/>";
    xml += "<w:pPr><w:spacing w:after=\"160\" w:line=\"259\" w:lineRule=\"auto\"/></w:pPr>";
    xml += "<w:rPr><w:rFonts w:ascii=\"Calibri\" w:hAnsi=\"Calibri\"/><w:sz w:val=\"22\"/></w:rPr>";
    xml += "</w:style>";
    // Heading 1-9
    for (int i = 1; i <= 9; ++i) {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "<w:style w:type=\"paragraph\" w:styleId=\"Heading%d\">"
                      "<w:name w:val=\"heading %d\"/>"
                      "<w:basedOn w:val=\"Normal\"/>"
                      "<w:next w:val=\"Normal\"/>"
                      "<w:qFormat/>"
                      "<w:pPr><w:keepNext/><w:spacing w:before=\"240\" w:after=\"60\"/>"
                      "<w:outlineLvl w:val=\"%d\"/></w:pPr>"
                      "<w:rPr><w:b/><w:sz w:val=\"%d\"/></w:rPr>"
                      "</w:style>",
                      i, i, i - 1,
                      32 - (i - 1) * 2);  // 字号递减：32,30,28...
        xml += buf;
    }
    xml += "</w:styles>";
    return xml;
}

// docProps/core.xml
std::string WmlWriter::writeCoreProperties(const WriteCoreProperties& props) {
    std::string xml;
    xml.reserve(512);
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    xml += "<cp:coreProperties "
           "xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
           "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
           "xmlns:dcterms=\"http://purl.org/dc/terms/\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">";
    if (!props.title.empty()) {
        xml += "<dc:title>";
        xml += escapeXml(props.title);
        xml += "</dc:title>";
    }
    if (!props.creator.empty()) {
        xml += "<dc:creator>";
        xml += escapeXml(props.creator);
        xml += "</dc:creator>";
    }
    if (!props.created.empty()) {
        xml += "<dcterms:created xsi:type=\"dcterms:W3CDTF\">";
        xml += escapeXml(props.created);
        xml += "</dcterms:created>";
    }
    if (!props.modified.empty()) {
        xml += "<dcterms:modified xsi:type=\"dcterms:W3CDTF\">";
        xml += escapeXml(props.modified);
        xml += "</dcterms:modified>";
    }
    xml += "</cp:coreProperties>";
    return xml;
}

} // namespace wml
} // namespace ooxml
} // namespace office
} // namespace zq
