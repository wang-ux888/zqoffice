// =============================================================================
// src/ooxml/wml/wml_reader.cpp
//
// WmlReader 实现：解析 WordprocessingML XML
// =============================================================================
#include "wml_reader.h"

#include <cstdlib>
#include <functional>

namespace zq {
namespace office {
namespace ooxml {
namespace wml {

namespace {

/// 字符串转 int32
std::int32_t toInt32_(const std::string& s) {
    if (s.empty()) return 0;
    try {
        return static_cast<std::int32_t>(std::stoi(s));
    } catch (...) {
        return 0;
    }
}

/// 字符串转 int64
std::int64_t toInt64_(const std::string& s) {
    if (s.empty()) return 0;
    try {
        return std::stoll(s);
    } catch (...) {
        return 0;
    }
}

} // namespace

// ===========================================================================
// 工具函数
// ===========================================================================

std::string WmlReader::localName_(const std::string& qualified) {
    std::size_t pos = qualified.find(':');
    if (pos == std::string::npos) return qualified;
    return qualified.substr(pos + 1);
}

std::string WmlReader::nodeLocalName_(const XmlNode& node) {
    if (!node.valid()) return "";
    return localName_(node.name());
}

// ===========================================================================
// 公共解析入口
// ===========================================================================

bool WmlReader::parseDocument(const std::string& xml) {
    parsed_ = false;
    blocks_.clear();
    error_.clear();

    if (!xmlReader_.parse(xml)) {
        error_ = "parse XML failed: " + xmlReader_.error();
        return false;
    }

    XmlNode root = xmlReader_.root();
    if (!root.valid()) {
        error_ = "no root element";
        return false;
    }

    if (nodeLocalName_(root) != "document") {
        error_ = "expected w:document, got " + root.name();
        return false;
    }

    // 查找 w:body
    XmlNode body;
    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "body") {
            body = child;
            break;
        }
    }
    if (!body.valid()) {
        error_ = "no w:body element";
        return false;
    }

    parseBody_(body);

    parsed_ = true;
    return true;
}

bool WmlReader::parseStyles(const std::string& xml) {
    styleNames_.clear();

    XmlReader reader;
    if (!reader.parse(xml)) {
        return false;
    }

    XmlNode root = reader.root();
    if (!root.valid()) return false;
    if (nodeLocalName_(root) != "styles") return false;

    // 遍历 w:style 子节点，提取 w:name@w:val
    for (XmlNode styleNode = root.firstChild("w:style"); styleNode.valid();
         styleNode = styleNode.nextSibling("w:style")) {
        // 注意：pugixml firstChild("w:style") 按限定名匹配，可能因前缀不同失效
        // 改用遍历所有子节点 + localName 判断
    }
    // 重新遍历（兼容无前缀情况）
    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) != "style") continue;
        // 查找 w:name 子节点
        for (XmlNode nameNode = child.firstChild(); nameNode.valid();
             nameNode = nameNode.nextSibling()) {
            if (nodeLocalName_(nameNode) == "name") {
                std::string val = nameNode.attribute("w:val");
                if (val.empty()) val = nameNode.attribute("val");
                if (!val.empty()) {
                    styleNames_.push_back(val);
                }
                break;
            }
        }
    }
    return true;
}

// ===========================================================================
// 内部解析
// ===========================================================================

void WmlReader::parseBody_(const XmlNode& body) {
    for (XmlNode child = body.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "p") {
            WmlBlock block;
            block.type = WmlBlockType::Paragraph;
            block.paragraph = parseParagraph_(child);

            // 检测段落内是否包含图片（w:r/w:drawing）
            // 若包含图片，则改写为 Image block
            WmlImage img;
            bool hasImage = false;
            for (XmlNode rNode = child.firstChild(); rNode.valid() && !hasImage;
                 rNode = rNode.nextSibling()) {
                if (nodeLocalName_(rNode) != "r") continue;
                for (XmlNode inR = rNode.firstChild(); inR.valid() && !hasImage;
                     inR = inR.nextSibling()) {
                    if (nodeLocalName_(inR) == "drawing") {
                        if (parseDrawing_(inR, img)) {
                            hasImage = true;
                        }
                    }
                }
            }

            if (hasImage) {
                block.type = WmlBlockType::Image;
                block.image = img;
                // 保留段落文本（部分图片段落也有文本）
                // 这里简化：图片 block 不带文本
            }
            blocks_.push_back(std::move(block));
        } else if (lname == "tbl") {
            WmlBlock block;
            block.type = WmlBlockType::Table;
            block.table = parseTable_(child);
            blocks_.push_back(std::move(block));
        }
        // 其他类型（sectPr 等）跳过
    }
}

WmlParagraph WmlReader::parseParagraph_(const XmlNode& pNode) {
    WmlParagraph para;

    // 解析 w:pPr（段落属性）
    for (XmlNode child = pNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) != "pPr") continue;

        // w:pStyle@w:val
        for (XmlNode pStyle = child.firstChild(); pStyle.valid();
             pStyle = pStyle.nextSibling()) {
            std::string l = nodeLocalName_(pStyle);
            if (l == "pStyle") {
                std::string v = pStyle.attribute("w:val");
                if (v.empty()) v = pStyle.attribute("val");
                para.style = v;
            } else if (l == "jc") {
                std::string v = pStyle.attribute("w:val");
                if (v.empty()) v = pStyle.attribute("val");
                para.alignment = v;
            }
        }
        break;
    }

    // 解析所有 w:r（run）
    for (XmlNode child = pNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) != "r") continue;
        para.runs.push_back(parseRun_(child));
    }

    return para;
}

WmlRun WmlReader::parseRun_(const XmlNode& rNode) {
    WmlRun run;

    // 解析 w:rPr（run 属性）
    for (XmlNode child = rNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "rPr") {
            run.font = parseRPr_(child);
            break;
        }
    }

    // 提取所有 w:t 文本（合并多个 w:t）
    for (XmlNode child = rNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string l = nodeLocalName_(child);
        if (l == "t") {
            run.text += child.text();
        } else if (l == "tab") {
            run.text += "\t";
        } else if (l == "br") {
            run.text += "\n";
        }
    }

    return run;
}

WmlFont WmlReader::parseRPr_(const XmlNode& rPrNode) {
    WmlFont font;
    if (!rPrNode.valid()) return font;

    for (XmlNode child = rPrNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string l = nodeLocalName_(child);
        if (l == "b") {
            font.bold = true;
        } else if (l == "i") {
            font.italic = true;
        } else if (l == "u") {
            font.underline = true;
        } else if (l == "sz") {
            std::string v = child.attribute("w:val");
            if (v.empty()) v = child.attribute("val");
            font.size = toInt32_(v);
        } else if (l == "rFonts") {
            std::string v = child.attribute("w:ascii");
            if (v.empty()) v = child.attribute("ascii");
            font.family = v;
        } else if (l == "color") {
            std::string v = child.attribute("w:val");
            if (v.empty()) v = child.attribute("val");
            font.color = v;
        }
    }
    return font;
}

WmlTable WmlReader::parseTable_(const XmlNode& tblNode) {
    WmlTable table;

    std::int32_t rowIndex = 0;
    std::int32_t maxCol = 0;

    for (XmlNode child = tblNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) != "tr") continue;

        std::int32_t colIndex = 0;
        for (XmlNode tcNode = child.firstChild(); tcNode.valid();
             tcNode = tcNode.nextSibling()) {
            if (nodeLocalName_(tcNode) != "tc") continue;

            WmlTableCell cell;
            cell.row = rowIndex;
            cell.column = colIndex;

            // 解析 w:tcPr@w:gridSpan
            for (XmlNode tcPr = tcNode.firstChild(); tcPr.valid();
                 tcPr = tcPr.nextSibling()) {
                if (nodeLocalName_(tcPr) != "tcPr") continue;
                for (XmlNode gs = tcPr.firstChild(); gs.valid();
                     gs = gs.nextSibling()) {
                    if (nodeLocalName_(gs) == "gridSpan") {
                        std::string v = gs.attribute("w:val");
                        if (v.empty()) v = gs.attribute("val");
                        cell.gridSpan = toInt32_(v);
                        if (cell.gridSpan < 1) cell.gridSpan = 1;
                        break;
                    }
                }
                break;
            }

            // 提取单元格内所有 run 文本（简化：拼接所有 w:t）
            for (XmlNode p = tcNode.firstChild(); p.valid();
                 p = p.nextSibling()) {
                if (nodeLocalName_(p) != "p") continue;
                WmlParagraph para = parseParagraph_(p);
                for (auto& run : para.runs) {
                    if (!cell.text.empty() && !run.text.empty()) {
                        cell.text += " ";
                    }
                    cell.text += run.text;
                }
            }

            table.cells.push_back(std::move(cell));
            colIndex += cell.gridSpan;
            if (colIndex > maxCol) maxCol = colIndex;
        }

        ++rowIndex;
    }

    table.rows = rowIndex;
    table.columns = maxCol;
    return table;
}

bool WmlReader::parseDrawing_(const XmlNode& drawingNode, WmlImage& outImage) {
    // 遍历查找 a:blip（r:embed）+ a:ext（cx/cy）
    bool found = false;

    // 递归查找（drawing → inline/anchor → graphic → graphicData → pic → blipFill/blip）
    // 简化：递归遍历所有子孙节点
    std::function<void(const XmlNode&)> visit = [&](const XmlNode& node) {
        if (!node.valid() || found) return;
        std::string l = nodeLocalName_(node);
        if (l == "blip") {
            std::string embed = node.attribute("r:embed");
            if (embed.empty()) embed = node.attribute("embed");
            if (!embed.empty()) {
                outImage.relId = embed;
                found = true;
            }
        }
        if (l == "ext" || l == "extent") {
            // a:ext（DrawingML）和 wp:extent（WordprocessingDrawing）均带 cx/cy
            std::string cx = node.attribute("cx");
            std::string cy = node.attribute("cy");
            if (!cx.empty()) outImage.width = toInt64_(cx);
            if (!cy.empty()) outImage.height = toInt64_(cy);
        }
        for (XmlNode child = node.firstChild(); child.valid();
             child = child.nextSibling()) {
            if (found) break;
            visit(child);
        }
    };

    visit(drawingNode);
    return found;
}

} // namespace wml
} // namespace ooxml
} // namespace office
} // namespace zq
