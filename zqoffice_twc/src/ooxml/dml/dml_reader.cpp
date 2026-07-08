// =============================================================================
// src/ooxml/dml/dml_reader.cpp
// =============================================================================
#include "dml_reader.h"

#include <cstdlib>
#include <sstream>

namespace zq {
namespace office {
namespace ooxml {
namespace dml {

namespace {

/// 字符串转 int64（容错，失败返回 0）
std::int64_t toInt64_(const std::string& s) {
    if (s.empty()) return 0;
    try {
        return std::stoll(s);
    } catch (...) {
        return 0;
    }
}

/// 字符串转 int32（容错，失败返回 0）
std::int32_t toInt32_(const std::string& s) {
    if (s.empty()) return 0;
    try {
        return static_cast<std::int32_t>(std::stoi(s));
    } catch (...) {
        return 0;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// 公共解析入口
// ---------------------------------------------------------------------------

bool DmlReader::parseSpreadsheetDrawing(const std::string& xml) {
    parsed_ = false;
    root_.reset();
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

    // xdr:wsDr 根节点：直接子节点应为 xdr:twoCellAnchor / xdr:oneCellAnchor /
    // xdr:absoluteAnchor
    root_ = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kGroup);
    root_->setName("wsDr-root");

    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "twoCellAnchor" || lname == "oneCellAnchor" ||
            lname == "absoluteAnchor") {
            auto anchor = parseAnchor_(child);
            if (anchor) {
                root_->addChild(std::move(anchor));
            }
        }
    }

    parsed_ = true;
    return true;
}

bool DmlReader::parseWordDrawing(const std::string& xml) {
    parsed_ = false;
    root_.reset();
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

    // Word drawing 顶层可能是 wpg:wgp 或 wps:wsp 或 a:graphic
    root_ = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kGroup);
    root_->setName("word-drawing-root");

    std::string rootLocal = nodeLocalName_(root);
    if (rootLocal == "wgp") {
        // 直接是组
        auto group = parseGroup_(root);
        if (group) {
            root_->addChild(std::move(group));
        }
    } else if (rootLocal == "wsp") {
        // 单个形状
        auto shape = parseShape_(root);
        if (shape) {
            root_->addChild(std::move(shape));
        }
    } else {
        // 通用：递归遍历所有子节点
        for (XmlNode child = root.firstChild(); child.valid();
             child = child.nextSibling()) {
            auto node = parseGeneric_(child);
            if (node) {
                root_->addChild(std::move(node));
            }
        }
    }

    parsed_ = true;
    return true;
}

bool DmlReader::parsePresentationDrawing(const std::string& xml) {
    parsed_ = false;
    root_.reset();
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

    // 幻灯片形状树根：p:spTree
    // 若直接传入整个 slideN.xml，需先定位 spTree
    XmlNode spTree = root;
    if (nodeLocalName_(root) != "spTree") {
        // 搜索 p:spTree
        // p:spTree 在 p:cSld 下
        XmlNode cSld = root.firstChild("p:cSld");
        if (cSld.valid()) {
            spTree = cSld.firstChild("p:spTree");
            if (!spTree.valid()) {
                // 尝试无前缀
                spTree = cSld.firstChild("spTree");
            }
        }
        if (!spTree.valid()) {
            spTree = root;  // 退化：把根当 spTree 处理
        }
    }

    root_ = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kSlideTree);
    root_->setName("spTree-root");
    root_->setXmlNode(spTree);

    // spTree 的子节点：nvGrpSpPr + grpSpPr + sp/pic/grpSp/cxnSp/graphicFrame
    for (XmlNode child = spTree.firstChild(); child.valid();
         child = child.nextSibling()) {
        auto node = parseGeneric_(child);
        if (node) {
            root_->addChild(std::move(node));
        }
    }

    parsed_ = true;
    return true;
}

bool DmlReader::parseChart(const std::string& xml) {
    chartParsed_ = false;
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

    chartParsed_ = true;
    return true;
}

XmlNode DmlReader::chartRoot() const {
    if (!chartParsed_) return XmlNode();
    return xmlReader_.root();
}

bool DmlReader::parseTheme(const std::string& xml) {
    themeParsed_ = false;
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

    themeParsed_ = true;
    return true;
}

XmlNode DmlReader::themeRoot() const {
    if (!themeParsed_) return XmlNode();
    return xmlReader_.root();
}

bool DmlReader::parseDiagram(const std::string& xml) {
    // Diagram 解析保留原始 XML，不做结构化转换
    parsed_ = false;
    root_.reset();
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

    // 仅创建一个虚拟根节点保留原始 XML
    root_ = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kUnknown);
    root_->setName("diagram-root");
    root_->setXmlNode(root);

    parsed_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// 通用节点分发
// ---------------------------------------------------------------------------

GroupShapeNode::Ptr DmlReader::parseGeneric_(const XmlNode& node) {
    if (!node.valid()) return nullptr;

    std::string lname = nodeLocalName_(node);
    if (lname == "sp") {
        return parseShape_(node);
    } else if (lname == "pic") {
        return parsePicture_(node);
    } else if (lname == "grpSp") {
        return parseGroup_(node);
    } else if (lname == "cxnSp") {
        return parseConnector_(node);
    } else if (lname == "graphicFrame") {
        return parseGraphicFrame_(node);
    }
    // 其他元素（如 nvGrpSpPr/grpSpPr）跳过
    return nullptr;
}

GroupShapeNode::Ptr DmlReader::parseShape_(const XmlNode& node) {
    auto shape = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kShape);
    shape->setXmlNode(node);

    // nvSpPr/cNvPr：id + name
    XmlNode nvSpPr = node.firstChild("nvSpPr");
    if (!nvSpPr.valid()) {
        // 尝试无前缀（PowerPoint 风格 p:nvSpPr 也匹配本地名 nvSpPr）
        // 但 firstChild 已经过滤了完整名，这里用遍历
        for (XmlNode child = node.firstChild(); child.valid();
             child = child.nextSibling()) {
            if (nodeLocalName_(child) == "nvSpPr") {
                nvSpPr = child;
                break;
            }
        }
    }
    if (nvSpPr.valid()) {
        // cNvPr
        for (XmlNode child = nvSpPr.firstChild(); child.valid();
             child = child.nextSibling()) {
            if (nodeLocalName_(child) == "cNvPr") {
                shape->setId(child.attribute("id"));
                shape->setName(child.attribute("name"));
                break;
            }
        }
    }

    // spPr：变换 + 几何 + 填充 + 轮廓
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string childLocal = nodeLocalName_(child);
        if (childLocal == "spPr") {
            shape->properties().hasSpPr = true;
            parseSpPr_(child, shape->properties());
            break;
        }
    }

    return shape;
}

GroupShapeNode::Ptr DmlReader::parsePicture_(const XmlNode& node) {
    auto pic = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kPicture);
    pic->setXmlNode(node);

    // nvPicPr/cNvPr：id + name
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string childLocal = nodeLocalName_(child);
        if (childLocal == "nvPicPr") {
            for (XmlNode nvChild = child.firstChild(); nvChild.valid();
                 nvChild = nvChild.nextSibling()) {
                if (nodeLocalName_(nvChild) == "cNvPr") {
                    pic->setId(nvChild.attribute("id"));
                    pic->setName(nvChild.attribute("name"));
                    break;
                }
            }
            break;
        }
    }

    // spPr
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "spPr") {
            pic->properties().hasSpPr = true;
            parseSpPr_(child, pic->properties());
            break;
        }
    }

    return pic;
}

GroupShapeNode::Ptr DmlReader::parseGroup_(const XmlNode& node) {
    auto group = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kGroup);
    group->setXmlNode(node);

    // nvGrpSpPr/cNvPr
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "nvGrpSpPr") {
            for (XmlNode nvChild = child.firstChild(); nvChild.valid();
                 nvChild = nvChild.nextSibling()) {
                if (nodeLocalName_(nvChild) == "cNvPr") {
                    group->setId(nvChild.attribute("id"));
                    group->setName(nvChild.attribute("name"));
                    break;
                }
            }
            break;
        }
    }

    // grpSpPr
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "grpSpPr") {
            group->properties().hasSpPr = true;
            parseSpPr_(child, group->properties());
            break;
        }
    }

    // 递归子节点
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        auto sub = parseGeneric_(child);
        if (sub) {
            group->addChild(std::move(sub));
        }
    }

    return group;
}

GroupShapeNode::Ptr DmlReader::parseConnector_(const XmlNode& node) {
    auto cxn = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kConnector);
    cxn->setXmlNode(node);

    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "nvCxnSpPr") {
            for (XmlNode nvChild = child.firstChild(); nvChild.valid();
                 nvChild = nvChild.nextSibling()) {
                if (nodeLocalName_(nvChild) == "cNvPr") {
                    cxn->setId(nvChild.attribute("id"));
                    cxn->setName(nvChild.attribute("name"));
                    break;
                }
            }
            break;
        }
    }

    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "spPr") {
            cxn->properties().hasSpPr = true;
            parseSpPr_(child, cxn->properties());
            break;
        }
    }

    return cxn;
}

GroupShapeNode::Ptr DmlReader::parseGraphicFrame_(const XmlNode& node) {
    auto gf = std::make_unique<GroupShapeNode>(GroupShapeNodeType::kGraphicFrame);
    gf->setXmlNode(node);

    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "nvGraphicFramePr") {
            for (XmlNode nvChild = child.firstChild(); nvChild.valid();
                 nvChild = nvChild.nextSibling()) {
                if (nodeLocalName_(nvChild) == "cNvPr") {
                    gf->setId(nvChild.attribute("id"));
                    gf->setName(nvChild.attribute("name"));
                    break;
                }
            }
            break;
        }
    }

    // graphicFrame 也有 xfrm（无 spPr 容器）
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "xfrm") {
            gf->properties().hasXfrm = true;
            parseXfrm_(child, gf->properties().xfrm);
            break;
        }
    }

    return gf;
}

GroupShapeNode::Ptr DmlReader::parseAnchor_(const XmlNode& node) {
    GroupShapeNodeType anchorType = GroupShapeNodeType::kUnknown;
    std::string lname = nodeLocalName_(node);
    if (lname == "twoCellAnchor") {
        anchorType = GroupShapeNodeType::kTwoCellAnchor;
    } else if (lname == "oneCellAnchor") {
        anchorType = GroupShapeNodeType::kOneCellAnchor;
    } else if (lname == "absoluteAnchor") {
        anchorType = GroupShapeNodeType::kAbsoluteAnchor;
    } else {
        return nullptr;
    }

    auto anchor = std::make_unique<GroupShapeNode>(anchorType);
    anchor->setXmlNode(node);
    anchor->setName(lname);

    // anchor 子节点：from/to/pos + sp/pic/grpSp/cxnSp/graphicFrame
    // 我们仅递归形状部分
    for (XmlNode child = node.firstChild(); child.valid();
         child = child.nextSibling()) {
        auto sub = parseGeneric_(child);
        if (sub) {
            anchor->addChild(std::move(sub));
        }
    }

    return anchor;
}

// ---------------------------------------------------------------------------
// 属性解析
// ---------------------------------------------------------------------------

void DmlReader::parseSpPr_(const XmlNode& spPr, ShapeProperties& props) {
    if (!spPr.valid()) return;
    props.hasSpPr = true;

    for (XmlNode child = spPr.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "xfrm") {
            props.hasXfrm = true;
            parseXfrm_(child, props.xfrm);
        } else if (lname == "prstGeom") {
            parsePrstGeom_(child, props.prstGeom);
        } else if (lname == "custGeom") {
            // 自定义几何，标记 prst 为 "custom"
            props.prstGeom.prst = "custom";
            props.prstGeom.valid = true;
        } else if (lname == "solidFill") {
            props.fill.type = FillType::kSolid;
            // 子节点应为 a:srgbClr 或 a:schemeClr
            for (XmlNode fc = child.firstChild(); fc.valid();
                 fc = fc.nextSibling()) {
                std::string fcLocal = nodeLocalName_(fc);
                if (fcLocal == "srgbClr") {
                    props.fill.color = fc.attribute("val");
                    // alpha
                    for (XmlNode alphaNode = fc.firstChild(); alphaNode.valid();
                         alphaNode = alphaNode.nextSibling()) {
                        if (nodeLocalName_(alphaNode) == "alpha") {
                            props.fill.alpha = toInt32_(alphaNode.attribute("val"));
                        }
                    }
                    break;
                } else if (fcLocal == "schemeClr") {
                    props.fill.color = "scheme:" + fc.attribute("val");
                    break;
                }
            }
        } else if (lname == "noFill") {
            props.fill.type = FillType::kNoFill;
        } else if (lname == "gradFill") {
            props.fill.type = FillType::kGradient;
        } else if (lname == "blipFill") {
            props.fill.type = FillType::kBlip;
        } else if (lname == "pattFill") {
            props.fill.type = FillType::kPattern;
        } else if (lname == "grpFill") {
            props.fill.type = FillType::kGroup;
        } else if (lname == "ln") {
            parseLine_(child, props.line);
        }
    }
}

void DmlReader::parseXfrm_(const XmlNode& xfrm, Transform& xfrm_out) {
    if (!xfrm.valid()) return;
    xfrm_out.valid = true;

    for (XmlNode child = xfrm.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "off") {
            xfrm_out.x = toInt64_(child.attribute("x"));
            xfrm_out.y = toInt64_(child.attribute("y"));
        } else if (lname == "ext") {
            xfrm_out.cx = toInt64_(child.attribute("cx"));
            xfrm_out.cy = toInt64_(child.attribute("cy"));
        } else if (lname == "chOff") {
            xfrm_out.chX = toInt64_(child.attribute("x"));
            xfrm_out.chY = toInt64_(child.attribute("y"));
            xfrm_out.hasChildOffset = true;
        } else if (lname == "chExt") {
            xfrm_out.chCx = toInt64_(child.attribute("cx"));
            xfrm_out.chCy = toInt64_(child.attribute("cy"));
            xfrm_out.hasChildOffset = true;
        }
    }
}

void DmlReader::parsePrstGeom_(const XmlNode& prstGeom, PresetGeometry& geom) {
    if (!prstGeom.valid()) return;
    geom.valid = true;
    geom.prst = prstGeom.attribute("prst");
    if (geom.prst.empty()) {
        geom.prst = "rect";  // 默认矩形
    }
}

void DmlReader::parseFill_(const XmlNode& spPr, Fill& fill) {
    // 占位（已在 parseSpPr_ 内联实现，保留接口以）
    (void)spPr;
    (void)fill;
}

void DmlReader::parseLine_(const XmlNode& ln, Line& line) {
    if (!ln.valid()) return;
    line.valid = true;

    // width 属性
    std::string w = ln.attribute("w");
    if (!w.empty()) {
        line.width = toInt32_(w);
    }

    // 子节点：noFill / solidFill / gradFill / prstDash / ...
    for (XmlNode child = ln.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "noFill") {
            line.noFill = true;
        } else if (lname == "solidFill") {
            for (XmlNode fc = child.firstChild(); fc.valid();
                 fc = fc.nextSibling()) {
                std::string fcLocal = nodeLocalName_(fc);
                if (fcLocal == "srgbClr") {
                    line.color = fc.attribute("val");
                    break;
                } else if (fcLocal == "schemeClr") {
                    line.color = "scheme:" + fc.attribute("val");
                    break;
                }
            }
        }
    }
}

std::string DmlReader::parseColor_(const XmlNode& colorNode) {
    if (!colorNode.valid()) return "";
    std::string local = nodeLocalName_(colorNode);
    if (local == "srgbClr") {
        return colorNode.attribute("val");
    } else if (local == "schemeClr") {
        return "scheme:" + colorNode.attribute("val");
    }
    return "";
}

// ---------------------------------------------------------------------------
// 工具
// ---------------------------------------------------------------------------

std::string DmlReader::localName_(const std::string& qualified) {
    std::size_t pos = qualified.find(':');
    if (pos == std::string::npos) return qualified;
    return qualified.substr(pos + 1);
}

bool DmlReader::matchLocalName_(const XmlNode& node,
                                  const std::string& localName) {
    if (!node.valid()) return false;
    return localName_(node.name()) == localName;
}

std::string DmlReader::nodeLocalName_(const XmlNode& node) {
    if (!node.valid()) return "";
    return localName_(node.name());
}

} // namespace dml
} // namespace ooxml
} // namespace office
} // namespace zq
