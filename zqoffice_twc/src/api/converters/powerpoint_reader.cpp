// =============================================================================
// src/api/converters/powerpoint_reader.cpp
//
// PowerPointReader 实现（Phase H-Part5 / H9）
//
// 流式读取 pptx 文件，按需解析单张 slide。复用 PptxConverter 的解析模式：
//   initialize → loadArchive_ → parsePresentation_ → parsePresentationRels_ → parseCoreProperties_
//   readNextSlide → parseSlide_ → extractShapes_ → buildShapeJson_ → parseNotes_
// =============================================================================
#include "powerpoint_reader.h"

#include <cstdlib>
#include <utility>

#include "zq/office/version.h"
#include "ooxml/dml/group_shape_node.h"
#include "ooxml/pml/pml_reader.h"
#include "ooxml/xml_reader.h"
#include "ooxml/zip_reader.h"

namespace zq {
namespace office {
namespace converters {

// ===========================================================================
// 内部工具
// ===========================================================================

namespace {

/// 提取 XML 限定名的 localName（去掉命名空间前缀）
std::string localName_(const std::string& qualified) {
    std::string::size_type colon = qualified.find(':');
    if (colon == std::string::npos) return qualified;
    return qualified.substr(colon + 1);
}

} // namespace

// ===========================================================================
// 构造 / 析构
// ===========================================================================

PowerPointReader::PowerPointReader() = default;
PowerPointReader::~PowerPointReader() = default;

// ===========================================================================
// 生命周期
// ===========================================================================

int PowerPointReader::initialize(const std::string& filePath, std::string* err) {
    if (filePath.empty()) {
        if (err) *err = "filePath is empty";
        return static_cast<int>(ErrorCode::InvalidArgument);
    }

    if (!loadArchive_(filePath)) {
        if (err) *err = err_.empty() ? "failed to open pptx" : err_;
        return static_cast<int>(ErrorCode::FileNotFound);
    }

    if (!parsePresentation_()) {
        if (err) *err = err_.empty() ? "failed to parse presentation.xml" : err_;
        return static_cast<int>(ErrorCode::ZipError);
    }

    if (!parsePresentationRels_()) {
        // rels 可选，失败不致命
    }

    if (!parseCoreProperties_()) {
        // coreProperties 可选，失败不致命
    }

    // 初始化当前索引为 -1（尚未开始读取）
    currentIndex_ = -1;

    return 0;
}

// ===========================================================================
// 流式读取
// ===========================================================================

std::string PowerPointReader::readNextSlide() {
    if (!zip_ || presentation_.slideIds.empty()) {
        return "";
    }

    int next = currentIndex_ + 1;
    if (next >= static_cast<int>(presentation_.slideIds.size())) {
        // 已到末尾
        return "";
    }

    std::string slideJson;
    if (!parseSlide_(next, &slideJson)) {
        // 单张 slide 解析失败，返回空（调用方可通过 currentIndex_ 推断）
        return "";
    }

    currentIndex_ = next;
    return slideJson;
}

bool PowerPointReader::seekToSlide(int index) {
    if (index < 0 || index >= static_cast<int>(presentation_.slideIds.size())) {
        return false;
    }
    // seekToSlide 将 currentIndex_ 设置为 index - 1，使得下一次 readNextSlide 读取 index
    currentIndex_ = index - 1;
    return true;
}

int PowerPointReader::slideCount() const {
    return static_cast<int>(presentation_.slideIds.size());
}

int PowerPointReader::currentIndex() const {
    return currentIndex_;
}

// ===========================================================================
// 选项与错误
// ===========================================================================

void PowerPointReader::setOption(int key, int value) {
    switch (key) {
        case 1:  // 是否提取图片
            extractImagesEnabled_ = (value != 0);
            break;
        case 2:  // 是否解析 notes
            parseNotesEnabled_ = (value != 0);
            break;
        default:
            // 未知选项忽略（与 IReader::SetOption 行为一致）
            break;
    }
}

const char* PowerPointReader::getErrorDescription(int err) {
    switch (err) {
        case 0:  return "ok";
        case static_cast<int>(ErrorCode::InvalidArgument):    return "invalid argument";
        case static_cast<int>(ErrorCode::FileNotFound):       return "file not found";
        case static_cast<int>(ErrorCode::InvalidFormat):      return "invalid format";
        case static_cast<int>(ErrorCode::UnsupportedFormat):  return "unsupported format";
        case static_cast<int>(ErrorCode::PasswordRequired):   return "password required";
        case static_cast<int>(ErrorCode::WrongPassword):      return "wrong password";
        case static_cast<int>(ErrorCode::CorruptedFile):      return "corrupted file";
        case static_cast<int>(ErrorCode::OutOfMemory):        return "out of memory";
        case static_cast<int>(ErrorCode::InternalError):      return "internal error";
        case static_cast<int>(ErrorCode::NotImplemented):     return "not implemented";
        case static_cast<int>(ErrorCode::EncodingError):      return "encoding error";
        case static_cast<int>(ErrorCode::XmlParseError):      return "xml parse error";
        case static_cast<int>(ErrorCode::ZipError):           return "zip error";
        case static_cast<int>(ErrorCode::Ole2Error):          return "ole2 error";
        case static_cast<int>(ErrorCode::BiffError):          return "biff error";
        case static_cast<int>(ErrorCode::EscherError):        return "escher error";
        case static_cast<int>(ErrorCode::EncryptionError):    return "encryption error";
        case static_cast<int>(ErrorCode::ConversionError):    return "conversion error";
        default: return "unknown error";
    }
}

// ===========================================================================
// 内部解析辅助
// ===========================================================================

bool PowerPointReader::loadArchive_(const std::string& filePath) {
    zip_ = std::make_unique<ooxml::ZipReader>();
    if (!zip_->open(filePath)) {
        err_ = "failed to open zip: " + filePath;
        return false;
    }
    pmlReader_ = std::make_unique<ooxml::pml::PmlReader>();
    return true;
}

bool PowerPointReader::parsePresentation_() {
    std::string presXml = zip_->readEntryText("ppt/presentation.xml");
    if (presXml.empty()) {
        err_ = "ppt/presentation.xml not found or empty";
        return false;
    }

    if (!pmlReader_->parsePresentation(presXml)) {
        err_ = "failed to parse presentation.xml: " + pmlReader_->error();
        return false;
    }

    // 提取 slideIds
    presentation_.slideIds.clear();
    for (const auto& sid : pmlReader_->slideIds()) {
        presentation_.slideIds.emplace_back(sid.id, sid.relId);
    }

    // 提取 slideSize
    const auto& sz = pmlReader_->slideSize();
    if (sz.valid) {
        presentation_.slideSizeCx = sz.cx;
        presentation_.slideSizeCy = sz.cy;
    }

    return true;
}

bool PowerPointReader::parsePresentationRels_() {
    std::string relsXml = zip_->readEntryText("ppt/_rels/presentation.xml.rels");
    if (relsXml.empty()) {
        return false;
    }

    // 简单字符串解析 <Relationship Id="..." Target="slides/slide1.xml" .../>
    presentation_.rels.clear();
    std::string::size_type pos = 0;
    while (true) {
        std::string::size_type relPos = relsXml.find("Relationship", pos);
        if (relPos == std::string::npos) break;

        std::string id;
        std::string::size_type idPos = relsXml.find("Id=\"", relPos);
        if (idPos != std::string::npos) {
            idPos += 4;
            std::string::size_type idEnd = relsXml.find('"', idPos);
            if (idEnd != std::string::npos) {
                id = relsXml.substr(idPos, idEnd - idPos);
            }
        }

        std::string target;
        std::string::size_type targetPos = relsXml.find("Target=\"", relPos);
        if (targetPos != std::string::npos) {
            targetPos += 8;
            std::string::size_type targetEnd = relsXml.find('"', targetPos);
            if (targetEnd != std::string::npos) {
                target = relsXml.substr(targetPos, targetEnd - targetPos);
            }
        }

        if (!id.empty() && !target.empty()) {
            // Target 是相对路径（如 "slides/slide1.xml"），转为 "ppt/slides/slide1.xml"
            std::string fullPath = target;
            if (fullPath.find("ppt/") != 0 && fullPath.find('/') != 0) {
                fullPath = "ppt/" + fullPath;
            }
            presentation_.rels.emplace_back(id, fullPath);
        }

        pos = relPos + 12;
    }

    return true;
}

bool PowerPointReader::parseCoreProperties_() {
    std::string coreXml = zip_->readEntryText("docProps/core.xml");
    if (coreXml.empty()) {
        return false;
    }

    // 简单提取 dc:title / dc:creator / dcterms:created / dcterms:modified
    auto extractTag = [&coreXml](const std::string& tag) -> std::string {
        std::string openTag = "<" + tag;
        std::string::size_type start = coreXml.find(openTag);
        if (start == std::string::npos) return "";
        // 跳过属性到 '>'
        std::string::size_type gt = coreXml.find('>', start);
        if (gt == std::string::npos) return "";
        std::string::size_type textStart = gt + 1;
        std::string closeTag = "</" + tag + ">";
        std::string::size_type end = coreXml.find(closeTag, textStart);
        if (end == std::string::npos) return "";
        return coreXml.substr(textStart, end - textStart);
    };

    coreProps_.title    = extractTag("dc:title");
    coreProps_.creator  = extractTag("dc:creator");
    coreProps_.created  = extractTag("dcterms:created");
    coreProps_.modified = extractTag("dcterms:modified");
    return true;
}

bool PowerPointReader::parseSlide_(int slideIndex, std::string* out) {
    if (slideIndex < 0 || slideIndex >= static_cast<int>(presentation_.slideIds.size())) {
        return false;
    }
    if (!out) return false;

    // 通过 rId 找到 slide 文件路径
    const auto& slideId = presentation_.slideIds[slideIndex];
    std::string slidePath;
    for (const auto& rel : presentation_.rels) {
        if (rel.first == slideId.second) {
            slidePath = rel.second;
            break;
        }
    }
    if (slidePath.empty()) {
        // 回退：尝试默认路径 ppt/slides/slideN.xml
        slidePath = "ppt/slides/slide" + std::to_string(slideIndex + 1) + ".xml";
    }

    if (!zip_->hasEntry(slidePath)) {
        err_ = "slide not found: " + slidePath;
        return false;
    }

    std::string slideXml = zip_->readEntryText(slidePath);
    if (slideXml.empty()) {
        err_ = "slide xml empty: " + slidePath;
        return false;
    }

    // PmlReader 复用
    pmlReader_ = std::make_unique<ooxml::pml::PmlReader>();
    if (!pmlReader_->parseSlide(slideXml)) {
        err_ = "failed to parse slide: " + pmlReader_->error();
        return false;
    }

    // 构建 slide JSON
    json::JsonValue slideObj = json::JsonValue::object();
    slideObj.set("slideId", std::string("slide_") + std::to_string(slideIndex));
    slideObj.set("index", static_cast<std::int64_t>(slideIndex));
    slideObj.set("layout", std::string(""));  // 暂不解析版式名称

    // shapes
    json::JsonValue shapesArr = json::JsonValue::array();
    if (pmlReader_->root()) {
        extractShapes_(*pmlReader_->root(), shapesArr);
    }
    slideObj.set("shapes", std::move(shapesArr));

    // notes
    std::string notes = parseNotesEnabled_ ? parseNotesSlide_(slideIndex) : std::string();
    slideObj.set("notes", notes);

    *out = json::serialize(slideObj);
    return true;
}

void PowerPointReader::extractShapes_(const ooxml::dml::GroupShapeNode& node,
                                         json::JsonValue& shapesArr) {
    // 遍历子节点
    for (const auto& child : node.children()) {
        if (!child) continue;

        // 如果是组节点，递归
        if (child->isGroup()) {
            extractShapes_(*child, shapesArr);
            continue;
        }

        // 叶子节点：构建 shape JSON
        json::JsonValue shapeJson = buildShapeJson_(*child);
        if (!shapeJson.isNull()) {
            shapesArr.append(std::move(shapeJson));
        }
    }
}

json::JsonValue PowerPointReader::buildShapeJson_(const ooxml::dml::GroupShapeNode& node) {
    using ooxml::dml::GroupShapeNodeType;

    json::JsonValue shape = json::JsonValue::object();

    // 基本信息
    shape.set("id", node.id().empty() ? std::string("shape") : node.id());
    shape.set("name", node.name());

    // 类型映射
    std::string typeStr;
    switch (node.type()) {
        case GroupShapeNodeType::kShape:
        case GroupShapeNodeType::kConnector:
            typeStr = "text";  // 简化：所有 sp 都当作 text
            break;
        case GroupShapeNodeType::kPicture:
            typeStr = "image";
            break;
        case GroupShapeNodeType::kGraphicFrame:
            typeStr = "chart";  // 简化：graphicFrame 当作 chart
            break;
        case GroupShapeNodeType::kGroup:
            typeStr = "group";
            break;
        default:
            typeStr = "text";
            break;
    }
    shape.set("type", typeStr);

    // 几何信息：从 XmlNode 提取 a:spPr/a:xfrm/a:off + a:ext
    const auto& xmlNode = node.xmlNode();
    json::JsonValue geometry = json::JsonValue::object();

    // 查找 spPr
    ooxml::XmlNode spPr;
    for (ooxml::XmlNode child = xmlNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = localName_(child.name());
        if (lname == "spPr") {
            spPr = child;
            break;
        }
    }

    std::int64_t x = 0, y = 0, cx = 0, cy = 0;
    std::int32_t rotation = 0;

    if (spPr.valid()) {
        // 查找 xfrm
        for (ooxml::XmlNode child = spPr.firstChild(); child.valid();
             child = child.nextSibling()) {
            std::string lname = localName_(child.name());
            if (lname == "xfrm") {
                // rot 属性
                std::string rotStr = child.attribute("rot");
                if (!rotStr.empty()) {
                    rotation = static_cast<std::int32_t>(std::atoll(rotStr.c_str()));
                }
                // off 和 ext
                for (ooxml::XmlNode sub = child.firstChild(); sub.valid();
                     sub = sub.nextSibling()) {
                    std::string subLname = localName_(sub.name());
                    if (subLname == "off") {
                        x = std::atoll(sub.attribute("x").c_str());
                        y = std::atoll(sub.attribute("y").c_str());
                    } else if (subLname == "ext") {
                        cx = std::atoll(sub.attribute("cx").c_str());
                        cy = std::atoll(sub.attribute("cy").c_str());
                    }
                }
                break;
            }
        }
    }

    geometry.set("x", static_cast<std::int64_t>(x));
    geometry.set("y", static_cast<std::int64_t>(y));
    geometry.set("width", static_cast<std::int64_t>(cx));
    geometry.set("height", static_cast<std::int64_t>(cy));
    shape.set("geometry", std::move(geometry));
    shape.set("rotation", static_cast<std::int64_t>(rotation));

    // 文本内容
    if (typeStr == "text" || typeStr == "autoshape") {
        shape.set("text", buildTextJson_(xmlNode));
    }

    // 填充颜色：从 spPr 提取 solidFill/srgbClr
    if (spPr.valid()) {
        std::string fillColor;
        for (ooxml::XmlNode child = spPr.firstChild(); child.valid();
             child = child.nextSibling()) {
            std::string lname = localName_(child.name());
            if (lname == "solidFill") {
                // 查找 srgbClr
                for (ooxml::XmlNode sub = child.firstChild(); sub.valid();
                     sub = sub.nextSibling()) {
                    if (localName_(sub.name()) == "srgbClr") {
                        fillColor = sub.attribute("val");
                        break;
                    }
                }
                break;
            }
        }
        json::JsonValue fill = json::JsonValue::object();
        if (!fillColor.empty()) {
            fill.set("type", std::string("solid"));
            fill.set("color", fillColor);
        } else {
            fill.set("type", std::string("none"));
        }
        shape.set("fill", std::move(fill));
    }

    // 图片路径（pic 节点）：从 blipFill/a:blip/@r:embed 提取 rId
    if (typeStr == "image") {
        std::string rId;
        for (ooxml::XmlNode child = xmlNode.firstChild(); child.valid();
             child = child.nextSibling()) {
            if (localName_(child.name()) == "blipFill") {
                for (ooxml::XmlNode sub = child.firstChild(); sub.valid();
                     sub = sub.nextSibling()) {
                    if (localName_(sub.name()) == "blip") {
                        // r:embed 属性（命名空间前缀可能是 r:embed 或 embed）
                        rId = sub.attribute("r:embed");
                        if (rId.empty()) {
                            rId = sub.attribute("embed");
                        }
                        break;
                    }
                }
                break;
            }
        }
        json::JsonValue imageObj = json::JsonValue::object();
        imageObj.set("path", rId.empty() ? std::string("") : std::string("media/") + rId);
        imageObj.set("width", static_cast<std::int64_t>(cx));
        imageObj.set("height", static_cast<std::int64_t>(cy));
        shape.set("image", std::move(imageObj));
    }

    return shape;
}

json::JsonValue PowerPointReader::buildTextJson_(const ooxml::XmlNode& spNode) {
    using ooxml::XmlNode;

    json::JsonValue textObj = json::JsonValue::object();
    json::JsonValue paragraphsArr = json::JsonValue::array();

    // 查找 txBody
    XmlNode txBody;
    for (XmlNode child = spNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (localName_(child.name()) == "txBody") {
            txBody = child;
            break;
        }
    }

    if (txBody.valid()) {
        // 遍历 a:p
        for (XmlNode p = txBody.firstChild(); p.valid();
             p = p.nextSibling()) {
            if (localName_(p.name()) != "p") continue;

            json::JsonValue para = json::JsonValue::object();

            // pPr（段落属性）
            std::string alignment = "left";
            for (XmlNode child = p.firstChild(); child.valid();
                 child = child.nextSibling()) {
                if (localName_(child.name()) == "pPr") {
                    std::string algn = child.attribute("algn");
                    if (algn == "ctr") alignment = "center";
                    else if (algn == "r") alignment = "right";
                    else if (algn == "just") alignment = "justify";
                    else alignment = "left";
                    break;
                }
            }
            para.set("alignment", alignment);

            // runs
            json::JsonValue runsArr = json::JsonValue::array();
            for (XmlNode child = p.firstChild(); child.valid();
                 child = child.nextSibling()) {
                if (localName_(child.name()) != "r") continue;

                json::JsonValue run = json::JsonValue::object();

                // rPr
                json::JsonValue font = json::JsonValue::object();
                std::string runText;
                for (XmlNode sub = child.firstChild(); sub.valid();
                     sub = sub.nextSibling()) {
                    std::string subLname = localName_(sub.name());
                    if (subLname == "rPr") {
                        std::string b = sub.attribute("b");
                        std::string i = sub.attribute("i");
                        std::string u = sub.attribute("u");
                        std::string sz = sub.attribute("sz");
                        if (b == "1") font.set("bold", true);
                        else font.set("bold", false);
                        if (i == "1") font.set("italic", true);
                        else font.set("italic", false);
                        if (!u.empty()) font.set("underline", true);
                        else font.set("underline", false);
                        if (!sz.empty()) {
                            // OOXML sz 单位是 1/100 pt
                            font.set("size", static_cast<std::int64_t>(std::atoll(sz.c_str())));
                        }
                        // 查找 latin typeface
                        for (XmlNode lat = sub.firstChild(); lat.valid();
                             lat = lat.nextSibling()) {
                            if (localName_(lat.name()) == "latin") {
                                font.set("family", lat.attribute("typeface"));
                                break;
                            }
                        }
                        // 查找 solidFill/srgbClr
                        for (XmlNode fl = sub.firstChild(); fl.valid();
                             fl = fl.nextSibling()) {
                            if (localName_(fl.name()) == "solidFill") {
                                for (XmlNode clr = fl.firstChild(); clr.valid();
                                     clr = clr.nextSibling()) {
                                    if (localName_(clr.name()) == "srgbClr") {
                                        font.set("color", clr.attribute("val"));
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    } else if (subLname == "t") {
                        runText = sub.text();
                    }
                }
                run.set("text", runText);
                run.set("font", std::move(font));
                runsArr.append(std::move(run));
            }
            para.set("runs", std::move(runsArr));

            paragraphsArr.append(std::move(para));
        }
    }

    textObj.set("paragraphs", std::move(paragraphsArr));
    return textObj;
}

std::string PowerPointReader::parseNotesSlide_(int slideIndex) {
    // notesSlide 路径通常为 ppt/notesSlides/notesSlideN.xml
    // 简化实现：直接尝试默认路径
    std::string notesPath = "ppt/notesSlides/notesSlide" +
                            std::to_string(slideIndex + 1) + ".xml";
    if (!zip_->hasEntry(notesPath)) {
        return "";
    }

    std::string notesXml = zip_->readEntryText(notesPath);
    if (notesXml.empty()) return "";

    // 简单提取所有 a:t 文本
    std::string text;
    std::string::size_type pos = 0;
    while (true) {
        std::string::size_type tStart = notesXml.find("<a:t>", pos);
        if (tStart == std::string::npos) break;
        tStart += 5;
        std::string::size_type tEnd = notesXml.find("</a:t>", tStart);
        if (tEnd == std::string::npos) break;
        if (!text.empty()) text += "\n";
        text += notesXml.substr(tStart, tEnd - tStart);
        pos = tEnd + 6;
    }

    return text;
}

} // namespace converters
} // namespace office
} // namespace zq
