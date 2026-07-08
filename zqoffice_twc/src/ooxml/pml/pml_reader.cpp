// =============================================================================
// src/ooxml/pml/pml_reader.cpp
// =============================================================================
#include "pml_reader.h"

#include "../dml/dml_reader.h"

#include <cstdlib>

namespace zq {
namespace office {
namespace ooxml {
namespace pml {

namespace {

std::int64_t toInt64_(const std::string& s) {
    if (s.empty()) return 0;
    try {
        return std::stoll(s);
    } catch (...) {
        return 0;
    }
}

std::uint32_t toUint32_(const std::string& s) {
    if (s.empty()) return 0;
    try {
        return static_cast<std::uint32_t>(std::stoul(s));
    } catch (...) {
        return 0;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// 公共解析入口
// ---------------------------------------------------------------------------

bool PmlReader::parseSlide(const std::string& xml) {
    parsed_ = false;
    root_.reset();
    error_.clear();
    slideType_ = SlideType::kUnknown;
    colorMap_ = ColorMap{};
    slideIds_.clear();
    slideSize_ = SlideSize{};

    if (!xmlReader_.parse(xml)) {
        error_ = "parse XML failed: " + xmlReader_.error();
        return false;
    }

    XmlNode root = xmlReader_.root();
    if (!root.valid()) {
        error_ = "no root element";
        return false;
    }

    if (nodeLocalName_(root) != "sld") {
        error_ = "expected p:sld, got " + root.name();
        return false;
    }

    return parseSlideLike_(root, SlideType::kSlide);
}

bool PmlReader::parseSlideLayout(const std::string& xml) {
    parsed_ = false;
    root_.reset();
    error_.clear();
    slideType_ = SlideType::kUnknown;
    colorMap_ = ColorMap{};
    slideIds_.clear();
    slideSize_ = SlideSize{};

    if (!xmlReader_.parse(xml)) {
        error_ = "parse XML failed: " + xmlReader_.error();
        return false;
    }

    XmlNode root = xmlReader_.root();
    if (!root.valid()) {
        error_ = "no root element";
        return false;
    }

    if (nodeLocalName_(root) != "sldLayout") {
        error_ = "expected p:sldLayout, got " + root.name();
        return false;
    }

    return parseSlideLike_(root, SlideType::kSlideLayout);
}

bool PmlReader::parseSlideMaster(const std::string& xml) {
    parsed_ = false;
    root_.reset();
    error_.clear();
    slideType_ = SlideType::kUnknown;
    colorMap_ = ColorMap{};
    slideIds_.clear();
    slideSize_ = SlideSize{};

    if (!xmlReader_.parse(xml)) {
        error_ = "parse XML failed: " + xmlReader_.error();
        return false;
    }

    XmlNode root = xmlReader_.root();
    if (!root.valid()) {
        error_ = "no root element";
        return false;
    }

    if (nodeLocalName_(root) != "sldMaster") {
        error_ = "expected p:sldMaster, got " + root.name();
        return false;
    }

    return parseSlideLike_(root, SlideType::kSlideMaster);
}

bool PmlReader::parsePresentation(const std::string& xml) {
    parsed_ = false;
    root_.reset();
    error_.clear();
    slideType_ = SlideType::kUnknown;
    colorMap_ = ColorMap{};
    slideIds_.clear();
    slideSize_ = SlideSize{};

    if (!xmlReader_.parse(xml)) {
        error_ = "parse XML failed: " + xmlReader_.error();
        return false;
    }

    XmlNode root = xmlReader_.root();
    if (!root.valid()) {
        error_ = "no root element";
        return false;
    }

    if (nodeLocalName_(root) != "presentation") {
        error_ = "expected p:presentation, got " + root.name();
        return false;
    }

    slideType_ = SlideType::kUnknown;  // presentation 不是幻灯片

    // 解析 sldSz 和 sldIdLst
    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "sldSz") {
            parseSlideSize_(child);
        } else if (lname == "sldIdLst") {
            parseSlideIdLst_(child);
        }
    }

    parsed_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// 内部解析
// ---------------------------------------------------------------------------

bool PmlReader::parseSlideLike_(const XmlNode& root, SlideType type) {
    slideType_ = type;

    // 查找 cSld（p:cSld）
    XmlNode cSld;
    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "cSld") {
            cSld = child;
            break;
        }
    }
    if (!cSld.valid()) {
        error_ = "no p:cSld element";
        return false;
    }

    // 解析 clrMap（仅 layout/master）
    // 注意：clrMap 在 p:sldLayout/p:sldMaster 中位于 cSld 之外（直接子节点）
    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "clrMap") {
            parseColorMap_(child);
            break;
        }
    }

    // 查找 spTree（p:cSld/p:spTree）
    XmlNode spTree;
    for (XmlNode child = cSld.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "spTree") {
            spTree = child;
            break;
        }
    }
    if (!spTree.valid()) {
        error_ = "no p:spTree element";
        return false;
    }

    // 复用 DmlReader 解析 spTree
    dml::DmlReader dmlReader;
    // 把 spTree 序列化为字符串再交给 DmlReader
    // 简化实现：直接使用 DmlReader 的内部逻辑
    // 由于 DmlReader.parsePresentationDrawing 接受完整 XML 字符串，
    // 我们重新构造一个包含 spTree 的 XML
    // 更高效的方式：让 DmlReader 暴露解析 XmlNode 的接口
    // 但为保持封装，这里通过字符串中转
    std::string spTreeXml = "<p:spTree xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">";
    // 注意：上面的中转方式会丢失原始命名空间声明，可能导致子节点前缀失效
    // 改用：直接在 PmlReader 内重新调用 DmlReader 但传入完整原始 XML
    (void)spTreeXml;

    // 实际方案：让 DmlReader 处理整个 slide XML（它会自动定位 spTree）
    // 但 DmlReader.parsePresentationDrawing 期望传入的是包含 spTree 的 XML
    // 我们已经把整个 XML 解析到 xmlReader_，但 DmlReader 有自己的 XmlReader
    // 所以最简单的办法：把原始 XML 传给 DmlReader
    // 这里我们改用：重新构造一个空的 XML 根，让 DmlReader 解析 cSld/spTree 子树

    // 由于 XmlNode 是视图，引用 xmlReader_ 的文档；
    // 我们使用 DmlReader 时需要给它一个独立的 XML 副本
    // 简化：复用 DmlReader 的逻辑，但通过把它放在 PmlReader 内部
    // 这里我们手动实现 spTree 解析（与 DmlReader.parsePresentationDrawing 相同逻辑）

    root_ = std::make_unique<dml::GroupShapeNode>(dml::GroupShapeNodeType::kSlideTree);
    root_->setName("spTree-root");
    root_->setXmlNode(spTree);

    // 遍历 spTree 子节点：sp/pic/grpSp/cxnSp/graphicFrame（其他跳过）
    for (XmlNode child = spTree.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        dml::GroupShapeNode::Ptr node;

        if (lname == "sp") {
            node = std::make_unique<dml::GroupShapeNode>(dml::GroupShapeNodeType::kShape);
        } else if (lname == "pic") {
            node = std::make_unique<dml::GroupShapeNode>(dml::GroupShapeNodeType::kPicture);
        } else if (lname == "grpSp") {
            node = std::make_unique<dml::GroupShapeNode>(dml::GroupShapeNodeType::kGroup);
        } else if (lname == "cxnSp") {
            node = std::make_unique<dml::GroupShapeNode>(dml::GroupShapeNodeType::kConnector);
        } else if (lname == "graphicFrame") {
            node = std::make_unique<dml::GroupShapeNode>(dml::GroupShapeNodeType::kGraphicFrame);
        }

        if (node) {
            node->setXmlNode(child);
            // 解析 id/name（cNvPr）
            for (XmlNode nvPr = child.firstChild(); nvPr.valid();
                 nvPr = nvPr.nextSibling()) {
                std::string nvLocal = nodeLocalName_(nvPr);
                if (nvLocal == "nvSpPr" || nvLocal == "nvPicPr" ||
                    nvLocal == "nvGrpSpPr" || nvLocal == "nvCxnSpPr" ||
                    nvLocal == "nvGraphicFramePr") {
                    for (XmlNode cNvPr = nvPr.firstChild(); cNvPr.valid();
                         cNvPr = cNvPr.nextSibling()) {
                        if (nodeLocalName_(cNvPr) == "cNvPr") {
                            node->setId(cNvPr.attribute("id"));
                            node->setName(cNvPr.attribute("name"));
                            break;
                        }
                    }
                    break;
                }
            }
            root_->addChild(std::move(node));
        }
    }

    parsed_ = true;
    return true;
}

void PmlReader::parseColorMap_(const XmlNode& clrMap) {
    if (!clrMap.valid()) return;
    colorMap_.valid = true;
    colorMap_.bg1 = clrMap.attribute("bg1");
    colorMap_.tx1 = clrMap.attribute("tx1");
    colorMap_.bg2 = clrMap.attribute("bg2");
    colorMap_.tx2 = clrMap.attribute("tx2");
    colorMap_.accent1 = clrMap.attribute("accent1");
    colorMap_.accent2 = clrMap.attribute("accent2");
    colorMap_.accent3 = clrMap.attribute("accent3");
    colorMap_.accent4 = clrMap.attribute("accent4");
    colorMap_.accent5 = clrMap.attribute("accent5");
    colorMap_.accent6 = clrMap.attribute("accent6");
    colorMap_.hlink = clrMap.attribute("hlink");
    colorMap_.folHlink = clrMap.attribute("folHlink");
}

void PmlReader::parseSlideSize_(const XmlNode& sldSz) {
    if (!sldSz.valid()) return;
    slideSize_.valid = true;
    slideSize_.cx = toInt64_(sldSz.attribute("cx"));
    slideSize_.cy = toInt64_(sldSz.attribute("cy"));
    slideSize_.type = sldSz.attribute("type");
}

void PmlReader::parseSlideIdLst_(const XmlNode& sldIdLst) {
    if (!sldIdLst.valid()) return;
    for (XmlNode child = sldIdLst.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "sldId") {
            SlideId sid;
            sid.id = toUint32_(child.attribute("id"));
            // r:id 属性
            sid.relId = child.attribute("r:id");
            if (sid.relId.empty()) {
                sid.relId = child.attribute("id");  // 容错：某些文件无 r: 前缀
            }
            slideIds_.push_back(sid);
        }
    }
}

// ---------------------------------------------------------------------------
// 工具
// ---------------------------------------------------------------------------

std::string PmlReader::localName_(const std::string& qualified) {
    std::size_t pos = qualified.find(':');
    if (pos == std::string::npos) return qualified;
    return qualified.substr(pos + 1);
}

std::string PmlReader::nodeLocalName_(const XmlNode& node) {
    if (!node.valid()) return "";
    return localName_(node.name());
}

} // namespace pml
} // namespace ooxml
} // namespace office
} // namespace zq
