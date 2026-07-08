// =============================================================================
// src/ooxml/sml/sml_writer.cpp
//
// SmlWriter 实现：生成 SpreadsheetML XML 部件
// =============================================================================
#include "sml_writer.h"

#include <sstream>
#include <string>

namespace zq {
namespace office {
namespace ooxml {
namespace sml {

// ===========================================================================
// 工具函数
// ===========================================================================

std::string SmlWriter::escapeXml(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;";  break;
            case '>':  out += "&gt;";  break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

std::string SmlWriter::columnToLetters(std::uint32_t col) {
    // 1 → "A", 26 → "Z", 27 → "AA"
    std::string result;
    while (col > 0) {
        col -= 1;  // 1-based → 0-based
        result.insert(result.begin(), static_cast<char>('A' + (col % 26)));
        col /= 26;
    }
    return result;
}

std::string SmlWriter::cellRef(std::uint32_t row, std::uint32_t col) {
    return columnToLetters(col) + std::to_string(row);
}

const char* SmlWriter::cellTypeAttr(WriteCellType t) {
    switch (t) {
        case WriteCellType::Number:        return "";    // 默认，不输出 t 属性
        case WriteCellType::SharedString:  return "s";
        case WriteCellType::Boolean:       return "b";
        case WriteCellType::InlineString:  return "inlineStr";
        case WriteCellType::Error:         return "e";
    }
    return "";
}

// ===========================================================================
// [Content_Types].xml
// ===========================================================================
std::string SmlWriter::writeContentTypes(std::size_t sheetCount,
                                          bool hasSharedStrings) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>";

    for (std::size_t i = 0; i < sheetCount; ++i) {
        xml += "<Override PartName=\"/xl/worksheets/sheet";
        xml += std::to_string(i + 1);
        xml += ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>";
    }

    if (hasSharedStrings) {
        xml += "<Override PartName=\"/xl/sharedStrings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml\"/>";
    }

    xml += "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>";
    xml += "</Types>";
    return xml;
}

// ===========================================================================
// _rels/.rels
// ===========================================================================
std::string SmlWriter::writeRootRels(bool hasCoreProps) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>";

    if (hasCoreProps) {
        xml += "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>";
    }

    xml += "</Relationships>";
    return xml;
}

// ===========================================================================
// xl/workbook.xml
// ===========================================================================
std::string SmlWriter::writeWorkbook(const std::vector<WriteSheetInfo>& sheets,
                                      const std::vector<WriteDefinedName>& names) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets>";

    for (const auto& s : sheets) {
        xml += "<sheet name=\"";
        xml += escapeXml(s.name);
        xml += "\" sheetId=\"";
        xml += std::to_string(s.sheetId);
        xml += "\" r:id=\"";
        xml += escapeXml(s.relId);
        xml += "\"/>";
    }

    xml += "</sheets>";

    // definedNames（可选）
    if (!names.empty()) {
        xml += "<definedNames>";
        for (const auto& dn : names) {
            xml += "<definedName name=\"";
            xml += escapeXml(dn.name);
            xml += "\">";
            xml += escapeXml(dn.ref);
            xml += "</definedName>";
        }
        xml += "</definedNames>";
    }

    xml += "</workbook>";
    return xml;
}

// ===========================================================================
// xl/_rels/workbook.xml.rels
// ===========================================================================
std::string SmlWriter::writeWorkbookRels(std::size_t sheetCount,
                                          bool hasSharedStrings,
                                          bool hasStyles) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";

    // 工作表关系 rId1..rIdN
    for (std::size_t i = 0; i < sheetCount; ++i) {
        xml += "<Relationship Id=\"rId";
        xml += std::to_string(i + 1);
        xml += "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet";
        xml += std::to_string(i + 1);
        xml += ".xml\"/>";
    }

    // styles 关系（rIdN+1）
    std::size_t nextId = sheetCount + 1;
    if (hasStyles) {
        xml += "<Relationship Id=\"rId";
        xml += std::to_string(nextId);
        xml += "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>";
        ++nextId;
    }

    // sharedStrings 关系
    if (hasSharedStrings) {
        xml += "<Relationship Id=\"rId";
        xml += std::to_string(nextId);
        xml += "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings\" Target=\"sharedStrings.xml\"/>";
    }

    xml += "</Relationships>";
    return xml;
}

// ===========================================================================
// xl/worksheets/sheetN.xml
// ===========================================================================
std::string SmlWriter::writeWorksheet(const WriteSheetData& data) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">";

    // sheetData
    xml += "<sheetData>";
    for (const auto& row : data.rows) {
        xml += "<row r=\"";
        xml += std::to_string(row.row);
        xml += "\">";

        for (const auto& cell : row.cells) {
            std::string ref = cellRef(cell.row, cell.col);
            xml += "<c r=\"";
            xml += ref;
            xml += "\"";

            // t 属性（Number 类型不输出）
            const char* t = cellTypeAttr(cell.type);
            if (t[0] != '\0') {
                xml += " t=\"";
                xml += t;
                xml += "\"";
            }

            // 值
            if (cell.type == WriteCellType::InlineString) {
                // 内联字符串：<is><t>...</t></is>
                xml += "><is><t>";
                xml += escapeXml(cell.value);
                xml += "</t></is></c>";
            } else if (!cell.value.empty()) {
                // 数字/共享字符串索引/布尔值：<v>...</v>
                xml += "><v>";
                xml += escapeXml(cell.value);
                xml += "</v></c>";
            } else {
                xml += "/>";
            }
        }

        xml += "</row>";
    }
    xml += "</sheetData>";

    // mergeCells（可选）
    if (!data.merges.empty()) {
        xml += "<mergeCells count=\"";
        xml += std::to_string(data.merges.size());
        xml += "\">";
        for (const auto& m : data.merges) {
            xml += "<mergeCell ref=\"";
            xml += cellRef(m.startRow, m.startCol);
            xml += ":";
            xml += cellRef(m.endRow, m.endCol);
            xml += "\"/>";
        }
        xml += "</mergeCells>";
    }

    xml += "</worksheet>";
    return xml;
}

// ===========================================================================
// xl/sharedStrings.xml
// ===========================================================================
std::string SmlWriter::writeSharedStrings(const std::vector<std::string>& strings) {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
       << "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
       << " count=\"" << strings.size() << "\""
       << " uniqueCount=\"" << strings.size() << "\">";
    for (const auto& s : strings) {
        ss << "<si><t>" << escapeXml(s) << "</t></si>";
    }
    ss << "</sst>";
    return ss.str();
}

// ===========================================================================
// xl/styles.xml（最小化：仅默认样式 + 1 个 cellXfs）
// ===========================================================================
std::string SmlWriter::writeStyles() {
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<fonts count=\"1\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>"
        "<fills count=\"1\"><fill><patternFill patternType=\"none\"/></fill></fills>"
        "<borders count=\"1\"><border/></borders>"
        "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
        "<cellXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/></cellXfs>"
        "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
        "</styleSheet>";
}

// ===========================================================================
// docProps/core.xml
// ===========================================================================
std::string SmlWriter::writeCoreProperties(const WriteCoreProperties& props) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
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

} // namespace sml
} // namespace ooxml
} // namespace office
} // namespace zq
