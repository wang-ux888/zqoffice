// =============================================================================
// src/drawing/tt_shape_convert.cpp
// =============================================================================
#include "tt_shape_convert.h"

#include "ooxml/xml_reader.h"
#include "drawing/enums/prst_shape_type.h"
#include "drawing/text_body.h"
#include "drawing/shape_pr.h"

namespace zq {
namespace office {
namespace drawing {

namespace {

/// 读取小端 16 位无符号整数
inline std::uint16_t readU16LE(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}

/// 读取小端 32 位有符号整数
inline std::int32_t readI32LE(const unsigned char* p) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(p[0]) |
        (static_cast<std::uint32_t>(p[1]) << 8) |
        (static_cast<std::uint32_t>(p[2]) << 16) |
        (static_cast<std::uint32_t>(p[3]) << 24));
}

} // namespace

TTShapeConvert::TTShapeConvert() = default;
TTShapeConvert::~TTShapeConvert() = default;

std::shared_ptr<TTShape> TTShapeConvert::convertFromOOXML(
    const ooxml::XmlNode& spNode) {
    success_ = false;
    error_.clear();

    if (!spNode.valid()) {
        error_ = "invalid sp node";
        return nullptr;
    }

    auto shape = std::make_shared<TTShape>();

    // 遍历 a:sp 子元素：nvSpPr/spPr/txBody/style
    for (ooxml::XmlNode child = spNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "spPr") {
            parseSpPr_(child, shape.get());
        } else if (ln == "txBody") {
            parseTxBody_(child, shape.get());
        } else if (ln == "nvSpPr") {
            // 非视觉属性，提取形状名称
            for (ooxml::XmlNode nvChild = child.firstChild(); nvChild.valid();
                 nvChild = nvChild.nextSibling()) {
                if (localName_(nvChild.name()) == "cNvPr") {
                    shape->setName(nvChild.attribute("name"));
                    break;
                }
            }
        }
    }

    success_ = true;
    return shape;
}

std::shared_ptr<TTShape> TTShapeConvert::convertPicFromOOXML(
    const ooxml::XmlNode& picNode) {
    success_ = false;
    error_.clear();

    if (!picNode.valid()) {
        error_ = "invalid pic node";
        return nullptr;
    }

    auto shape = std::make_shared<TTShape>();
    shape->setShapeType(PrstShapeType::kRect);  // 图片默认矩形

    // 遍历 a:pic 子元素
    for (ooxml::XmlNode child = picNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "spPr") {
            parseSpPr_(child, shape.get());
        } else if (ln == "txBody") {
            parseTxBody_(child, shape.get());
        }
    }

    success_ = true;
    return shape;
}

std::shared_ptr<TTShape> TTShapeConvert::convertConnectorFromOOXML(
    const ooxml::XmlNode& cxnSpNode) {
    success_ = false;
    error_.clear();

    if (!cxnSpNode.valid()) {
        error_ = "invalid cxnSp node";
        return nullptr;
    }

    auto shape = std::make_shared<TTShape>();
    shape->setShapeType(PrstShapeType::kStraightConnector1);

    for (ooxml::XmlNode child = cxnSpNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "spPr") {
            parseSpPr_(child, shape.get());
        }
    }

    success_ = true;
    return shape;
}

std::shared_ptr<TTShape> TTShapeConvert::convertFromBinary(
    const unsigned char* data, std::size_t size) {
    success_ = false;
    error_.clear();

    if (!data || size < 8) {
        error_ = "binary data too small";
        return nullptr;
    }

    // 解析 SpContainer 记录
    std::uint16_t recType = readU16LE(data + 2);
    if (recType != 0xF004) {  // SpContainer
        error_ = "not a SpContainer";
        return nullptr;
    }

    auto shape = std::make_shared<TTShape>();

    // 遍历子记录查找 ShapeProps (OPT) 和 ClientTextbox
    std::size_t offset = 8;
    std::uint32_t recLen = static_cast<std::uint32_t>(
        readU16LE(data + 4) | (readU16LE(data + 6) << 16));
    std::size_t end = 8 + recLen;
    if (end > size) end = size;

    while (offset + 8 <= end) {
        std::uint16_t subType = readU16LE(data + offset + 2);
        std::uint32_t subLen = static_cast<std::uint32_t>(
            readU16LE(data + offset + 4) | (readU16LE(data + offset + 6) << 16));

        if (offset + 8 + subLen > end) break;

        // OPT 记录 (0xF00B) 包含形状属性
        if (subType == 0xF00B && subLen >= 8) {
            // 简化：从 OPT 中读取位置和尺寸
            // 完整实现需要解析 OfficeArtFOPTE 数组
        }
        // ClientTextbox (0xF00D) 包含文本数据
        else if (subType == 0xF00D) {
            // 文本数据需要客户端特定解析
        }

        offset += 8 + subLen;
    }

    success_ = true;
    return shape;
}

bool TTShapeConvert::parseSpPr_(const ooxml::XmlNode& spPrNode, TTShape* shape) {
    if (!spPrNode.valid() || !shape) return false;

    for (ooxml::XmlNode child = spPrNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "xfrm") {
            // 解析变换：off/ext
            for (ooxml::XmlNode xfrmChild = child.firstChild();
                 xfrmChild.valid(); xfrmChild = xfrmChild.nextSibling()) {
                std::string xln = localName_(xfrmChild.name());
                if (xln == "off") {
                    shape->SetX(std::stoll(xfrmChild.attribute("x", "0")));
                    shape->SetY(std::stoll(xfrmChild.attribute("y", "0")));
                } else if (xln == "ext") {
                    shape->SetWidth(std::stoll(xfrmChild.attribute("cx", "0")));
                    shape->SetHeight(std::stoll(xfrmChild.attribute("cy", "0")));
                }
            }
        } else if (ln == "prstGeom") {
            std::string prst = child.attribute("prst", "rect");
            shape->setShapeType(prstShapeTypeFromString(prst));
        }
    }

    return true;
}

bool TTShapeConvert::parseTxBody_(const ooxml::XmlNode& txBodyNode,
                                    TTShape* shape) {
    if (!txBodyNode.valid() || !shape) return false;

    auto textBody = std::make_shared<TextBody>();

    // 遍历 a:txBody 子元素：bodyPr/p
    for (ooxml::XmlNode child = txBodyNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "bodyPr") {
            // 解析 bodyPr 属性
            std::string anchor = child.attribute("anchor", "t");
            textBody->setVerticalAlign(textAnchoringTypeFromString(anchor));

            std::string wrap = child.attribute("wrap", "square");
            textBody->setWrapText(textWrapTypeFromString(wrap));

            std::string vertOverflow = child.attribute("vertOverflow", "overflow");
            textBody->setVertClip(textVertOverflowTypeFromString(vertOverflow));

            std::string vert = child.attribute("vert", "horz");
            textBody->setTextFlow(textVerticalTypeFromString(vert));

            std::string horzOverflow = child.attribute("horzOverflow", "overflow");
            textBody->setHorzClip(textHorzOverflowTypeFromString(horzOverflow));
        } else if (ln == "p") {
            // 解析段落
            TextParagraph para;
            para.valid = true;

            for (ooxml::XmlNode pChild = child.firstChild(); pChild.valid();
                 pChild = pChild.nextSibling()) {
                std::string pln = localName_(pChild.name());

                if (pln == "r") {
                    // 文本运行
                    TextRun run;
                    run.valid = true;

                    for (ooxml::XmlNode rChild = pChild.firstChild();
                         rChild.valid(); rChild = rChild.nextSibling()) {
                        std::string rln = localName_(rChild.name());
                        if (rln == "rPr") {
                            // 运行属性
                            run.bold = rChild.attribute("b", "0") == "1" ||
                                       rChild.attribute("b", "false") == "true";
                            run.italic = rChild.attribute("i", "0") == "1" ||
                                         rChild.attribute("i", "false") == "true";
                            std::string sz = rChild.attribute("sz", "");
                            if (!sz.empty()) {
                                try {
                                    run.fontSize = std::stod(sz) / 100.0;
                                } catch (...) {}
                            }
                        } else if (rln == "t") {
                            run.text = rChild.text();
                        }
                    }

                    if (!run.text.empty() || run.valid) {
                        para.runs.push_back(run);
                    }
                }
            }

            textBody->addParagraph(para);
        }
    }

    shape->setTextBody(textBody);
    return true;
}

std::string TTShapeConvert::localName_(const std::string& fullName) {
    auto pos = fullName.find(':');
    if (pos == std::string::npos) return fullName;
    return fullName.substr(pos + 1);
}

} // namespace drawing
} // namespace office
} // namespace zq
