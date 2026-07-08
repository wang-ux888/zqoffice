// =============================================================================
// src/api/converters/pptx_converter.cpp
//
// PptxConverter 实现：
//   - 读取管线（H6）：pptx → ZQ Slide JSON
//   - 写入管线（H7）：ZQ Slide JSON → pptx
// =============================================================================
#include "pptx_converter.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <utility>

#include "zq/office/version.h"
#include "ooxml/dml/group_shape_node.h"
#include "ooxml/pml/pml_reader.h"
#include "ooxml/pml/pml_writer.h"
#include "ooxml/xml_reader.h"
#include "ooxml/zip_reader.h"
#include "ooxml/zip_writer.h"

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

PptxConverter::PptxConverter() = default;
PptxConverter::~PptxConverter() = default;

// ===========================================================================
// 公开 API：读取（pptx → JSON）
// ===========================================================================

std::string PptxConverter::toJson(const std::string& filePath,
                                  const std::string& dataDirectory,
                                  const std::string& optionsJson) {
    (void)dataDirectory;    // Phase H-Part3 暂不处理图片提取
    (void)optionsJson;      // Phase H-Part3 暂不解析选项

    if (!loadArchive_(filePath)) {
        return errorJson_(ErrorCode::FileNotFound,
                          err_.empty() ? "failed to open pptx" : err_);
    }

    if (!parsePresentation_()) {
        return errorJson_(ErrorCode::ZipError,
                          err_.empty() ? "failed to parse presentation.xml" : err_);
    }

    if (!parsePresentationRels_()) {
        // rels 可选，失败不致命
    }

    // 解析所有 slides
    slideResults_.clear();
    slideResults_.resize(presentation_.slideIds.size());
    for (std::size_t i = 0; i < presentation_.slideIds.size(); ++i) {
        if (!parseSlide_(static_cast<int>(i))) {
            // 单个 slide 解析失败，记录但继续
            slideResults_[i].notes.clear();
        }
    }

    // 组装最终 JSON
    json::JsonValue root = json::JsonValue::object();

    // slides 数组
    json::JsonValue slidesArr = json::JsonValue::array();
    for (std::size_t i = 0; i < presentation_.slideIds.size(); ++i) {
        json::JsonValue slideObj = json::JsonValue::object();
        slideObj.set("slideId", std::string("slide_") + std::to_string(i));
        slideObj.set("index", static_cast<std::int64_t>(i));

        // layout（Phase H-Part3 暂不解析版式名称，留空）
        slideObj.set("layout", std::string(""));

        // shapes
        json::JsonValue shapesArr = json::JsonValue::array();
        for (auto& shape : slideResults_[i].shapes) {
            shapesArr.append(shape);
        }
        slideObj.set("shapes", std::move(shapesArr));

        // notes
        slideObj.set("notes", slideResults_[i].notes);

        slidesArr.append(std::move(slideObj));
    }
    root.set("slides", std::move(slidesArr));

    // slideSize
    json::JsonValue sizeObj = json::JsonValue::object();
    // EMU → 1/100 pt（与 schema 示例对齐：960×540 是 10×5.625 inch 在 96dpi 下的像素）
    // 实际 schema 用 EMU，这里直接输出 EMU
    sizeObj.set("width", static_cast<std::int64_t>(presentation_.slideSizeCx));
    sizeObj.set("height", static_cast<std::int64_t>(presentation_.slideSizeCy));
    root.set("slideSize", std::move(sizeObj));

    // coreProperties
    json::JsonValue coreObj = json::JsonValue::object();
    coreObj.set("title", coreProps_.title);
    coreObj.set("creator", coreProps_.creator);
    coreObj.set("created", coreProps_.created);
    coreObj.set("modified", coreProps_.modified);
    root.set("coreProperties", std::move(coreObj));

    // 序列化内层 data
    std::string dataStr = json::serialize(root);

    // 构造外层 ExportResult
    json::JsonValue result = json::JsonValue::object();
    result.set("code", static_cast<std::int64_t>(ErrorCode::OK));
    result.set("message", std::string("ok"));
    result.set("data", dataStr);
    result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));

    return json::serialize(result);
}

// ===========================================================================
// 公开 API：写入（JSON → pptx）
// ===========================================================================

std::string PptxConverter::fromJson(const std::string& clientVarsJson,
                                    const std::string& outputPath,
                                    const std::string& dataDirectory,
                                    const std::string& optionsJson) {
    (void)dataDirectory;    // Phase H-Part3 暂不处理图片资源
    (void)optionsJson;      // Phase H-Part3 暂不解析选项

    if (outputPath.empty()) {
        return errorJson_(ErrorCode::InvalidArgument, "outputPath is empty");
    }

    if (clientVarsJson.empty()) {
        return errorJson_(ErrorCode::InvalidArgument, "clientVarsJson is empty");
    }

    // 1. 解析 ZQ Slide JSON
    if (!parseZQSlideJson_(clientVarsJson)) {
        return errorJson_(ErrorCode::ConversionError,
                          err_.empty() ? "failed to parse clientVars JSON" : err_);
    }

    // 2. 生成 XML 部件并用 ZipWriter 打包
    if (!writePptx_(outputPath)) {
        return errorJson_(ErrorCode::ZipError,
                          err_.empty() ? "failed to write pptx" : err_);
    }

    // 3. 返回成功结果
    json::JsonValue result = json::JsonValue::object();
    result.set("code", static_cast<std::int64_t>(ErrorCode::OK));
    result.set("message", std::string("ok"));
    result.set("data", std::string(""));
    result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
    return json::serialize(result);
}

// ===========================================================================
// 内部读取辅助
// ===========================================================================

bool PptxConverter::loadArchive_(const std::string& filePath) {
    zip_ = std::make_unique<ooxml::ZipReader>();
    if (!zip_->open(filePath)) {
        err_ = "failed to open zip: " + filePath;
        return false;
    }
    pmlReader_ = std::make_unique<ooxml::pml::PmlReader>();
    return true;
}

bool PptxConverter::parsePresentation_() {
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

bool PptxConverter::parsePresentationRels_() {
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

bool PptxConverter::parseSlide_(int slideIndex) {
    if (slideIndex < 0 || slideIndex >= static_cast<int>(presentation_.slideIds.size())) {
        return false;
    }

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

    // 提取 shapes
    auto& result = slideResults_[slideIndex];
    result.shapes.clear();

    json::JsonValue shapesArr = json::JsonValue::array();
    if (pmlReader_->root()) {
        extractShapes_(*pmlReader_->root(), shapesArr);
    }

    // 把 shapesArr 中的元素移到 result.shapes
    for (std::size_t i = 0; i < shapesArr.size(); ++i) {
        result.shapes.push_back(shapesArr.at(i));
    }

    // 解析 notes
    result.notes = parseNotes_(slideIndex);

    return true;
}

void PptxConverter::extractShapes_(const ooxml::dml::GroupShapeNode& node,
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

json::JsonValue PptxConverter::buildShapeJson_(const ooxml::dml::GroupShapeNode& node) {
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
    bool hasGeom = false;

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
                        hasGeom = true;
                    } else if (subLname == "ext") {
                        cx = std::atoll(sub.attribute("cx").c_str());
                        cy = std::atoll(sub.attribute("cy").c_str());
                        hasGeom = true;
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

json::JsonValue PptxConverter::buildTextJson_(const ooxml::XmlNode& spNode) {
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

std::string PptxConverter::parseNotes_(int slideIndex) {
    // notesSlide 路径通常为 ppt/notesSlides/notesSlideN.xml
    // 通过 slideN.xml.rels 找到 notesSlide rId
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

// ===========================================================================
// 内部写入辅助
// ===========================================================================

bool PptxConverter::parseZQSlideJson_(const std::string& clientVarsJson) {
    writeSlides_.clear();
    writeSlideSize_ = ooxml::pml::WriteSlideSize{};
    writeCore_ = ooxml::pml::WriteCoreProperties{};

    // 解析 JSON：可能是 ExportResult 包装，也可能是直接 data
    json::JsonValue root;
    try {
        root = json::parse(clientVarsJson);
    } catch (const std::exception& e) {
        err_ = "JSON parse error: ";
        err_ += e.what();
        return false;
    }

    // 检测 ExportResult 包装
    const json::JsonValue* dataPtr = &root;
    json::JsonValue innerData;
    if (root.isObject() && root.contains("code") && root.contains("data")) {
        const json::JsonValue* dataField = root.find("data");
        if (dataField && dataField->isString()) {
            try {
                innerData = json::parse(dataField->asString());
                dataPtr = &innerData;
            } catch (const std::exception& e) {
                err_ = "JSON parse error (inner data): ";
                err_ += e.what();
                return false;
            }
        } else if (dataField && dataField->isObject()) {
            dataPtr = dataField;
        }
    }

    if (!dataPtr->isObject()) {
        err_ = "clientVars JSON root is not an object";
        return false;
    }

    // 解析 slideSize（可选）
    const json::JsonValue* ss = dataPtr->find("slideSize");
    if (ss && ss->isObject()) {
        const json::JsonValue* w = ss->find("width");
        const json::JsonValue* h = ss->find("height");
        if (w && w->isNumber()) writeSlideSize_.width = w->asInt();
        if (h && h->isNumber()) writeSlideSize_.height = h->asInt();
    }

    // 解析 slides 数组
    const json::JsonValue* slidesArr = dataPtr->find("slides");
    if (!slidesArr || !slidesArr->isArray()) {
        err_ = "missing 'slides' array in clientVars JSON";
        return false;
    }

    for (std::size_t i = 0; i < slidesArr->size(); ++i) {
        const json::JsonValue& slideJson = slidesArr->at(i);
        if (!slideJson.isObject()) {
            err_ = "slide[";
            err_ += std::to_string(i);
            err_ += "] is not an object";
            return false;
        }

        ooxml::pml::WriteSlide slide;

        // 解析 shapes
        const json::JsonValue* shapesArr = slideJson.find("shapes");
        if (shapesArr && shapesArr->isArray()) {
            for (std::size_t j = 0; j < shapesArr->size(); ++j) {
                const json::JsonValue& shapeJson = shapesArr->at(j);
                if (!shapeJson.isObject()) continue;

                ooxml::pml::WriteShape shape;

                // id
                const json::JsonValue* id = shapeJson.find("id");
                if (id && id->isString()) shape.id = id->asString();

                // name
                const json::JsonValue* nm = shapeJson.find("name");
                if (nm && nm->isString()) shape.name = nm->asString();

                // type
                const json::JsonValue* tp = shapeJson.find("type");
                std::string typeStr;
                if (tp && tp->isString()) typeStr = tp->asString();
                if (typeStr == "text") shape.type = ooxml::pml::WriteShapeType::Text;
                else if (typeStr == "image") shape.type = ooxml::pml::WriteShapeType::Image;
                else if (typeStr == "chart") shape.type = ooxml::pml::WriteShapeType::Chart;
                else if (typeStr == "group") shape.type = ooxml::pml::WriteShapeType::Group;
                else if (typeStr == "autoshape") shape.type = ooxml::pml::WriteShapeType::AutoShape;
                else shape.type = ooxml::pml::WriteShapeType::Text;

                // geometry
                const json::JsonValue* geo = shapeJson.find("geometry");
                if (geo && geo->isObject()) {
                    const json::JsonValue* gx = geo->find("x");
                    const json::JsonValue* gy = geo->find("y");
                    const json::JsonValue* gw = geo->find("width");
                    const json::JsonValue* gh = geo->find("height");
                    if (gx && gx->isNumber()) shape.geometry.x = gx->asInt();
                    if (gy && gy->isNumber()) shape.geometry.y = gy->asInt();
                    if (gw && gw->isNumber()) shape.geometry.width = gw->asInt();
                    if (gh && gh->isNumber()) shape.geometry.height = gh->asInt();
                }

                // rotation
                const json::JsonValue* rot = shapeJson.find("rotation");
                if (rot && rot->isNumber()) shape.rotation = static_cast<std::int32_t>(rot->asInt());

                // 段落（text 类型）
                const json::JsonValue* textObj = shapeJson.find("text");
                if (textObj && textObj->isObject()) {
                    const json::JsonValue* paras = textObj->find("paragraphs");
                    if (paras && paras->isArray()) {
                        for (std::size_t k = 0; k < paras->size(); ++k) {
                            const json::JsonValue& paraJson = paras->at(k);
                            if (!paraJson.isObject()) continue;

                            ooxml::pml::WriteParagraph para;

                            // alignment
                            const json::JsonValue* algn = paraJson.find("alignment");
                            if (algn && algn->isString()) {
                                std::string a = algn->asString();
                                if (a == "center") para.alignment = ooxml::pml::WriteAlignment::Center;
                                else if (a == "right") para.alignment = ooxml::pml::WriteAlignment::Right;
                                else if (a == "justify") para.alignment = ooxml::pml::WriteAlignment::Justify;
                                else para.alignment = ooxml::pml::WriteAlignment::Left;
                            }

                            // runs
                            const json::JsonValue* runs = paraJson.find("runs");
                            if (runs && runs->isArray()) {
                                for (std::size_t r = 0; r < runs->size(); ++r) {
                                    const json::JsonValue& runJson = runs->at(r);
                                    if (!runJson.isObject()) continue;

                                    ooxml::pml::WriteRun run;

                                    const json::JsonValue* rt = runJson.find("text");
                                    if (rt && rt->isString()) run.text = rt->asString();

                                    const json::JsonValue* font = runJson.find("font");
                                    if (font && font->isObject()) {
                                        const json::JsonValue* ff = font->find("family");
                                        const json::JsonValue* fs = font->find("size");
                                        const json::JsonValue* fb = font->find("bold");
                                        const json::JsonValue* fi = font->find("italic");
                                        const json::JsonValue* fu = font->find("underline");
                                        const json::JsonValue* fc = font->find("color");
                                        if (ff && ff->isString()) run.font.family = ff->asString();
                                        if (fs && fs->isNumber()) run.font.size = static_cast<std::int32_t>(fs->asInt());
                                        if (fb && fb->isBool()) run.font.bold = fb->asBool();
                                        if (fi && fi->isBool()) run.font.italic = fi->asBool();
                                        if (fu && fu->isBool()) run.font.underline = fu->asBool();
                                        if (fc && fc->isString()) run.font.color = fc->asString();
                                    }

                                    para.runs.push_back(std::move(run));
                                }
                            }

                            shape.paragraphs.push_back(std::move(para));
                        }
                    }
                }

                // fillColor
                const json::JsonValue* fillObj = shapeJson.find("fill");
                if (fillObj && fillObj->isObject()) {
                    const json::JsonValue* ft = fillObj->find("type");
                    const json::JsonValue* fc = fillObj->find("color");
                    if (ft && ft->isString() && ft->asString() == "solid") {
                        if (fc && fc->isString()) shape.fillColor = fc->asString();
                    }
                }

                // imagePath（image 类型）
                if (shape.type == ooxml::pml::WriteShapeType::Image) {
                    const json::JsonValue* imgObj = shapeJson.find("image");
                    if (imgObj && imgObj->isObject()) {
                        const json::JsonValue* ipp = imgObj->find("path");
                        if (ipp && ipp->isString()) shape.imagePath = ipp->asString();
                    }
                }

                // chartType/chartTitle（chart 类型）
                if (shape.type == ooxml::pml::WriteShapeType::Chart) {
                    const json::JsonValue* chartObj = shapeJson.find("chart");
                    if (chartObj && chartObj->isObject()) {
                        const json::JsonValue* ct = chartObj->find("type");
                        const json::JsonValue* cttl = chartObj->find("title");
                        if (ct && ct->isString()) shape.chartType = ct->asString();
                        if (cttl && cttl->isString()) shape.chartTitle = cttl->asString();
                    }
                }

                slide.shapes.push_back(std::move(shape));
            }
        }

        // 解析 notes
        const json::JsonValue* notes = slideJson.find("notes");
        if (notes && notes->isString()) {
            slide.notes.text = notes->asString();
        }

        writeSlides_.push_back(std::move(slide));
    }

    // 解析 coreProperties（可选）
    const json::JsonValue* cp = dataPtr->find("coreProperties");
    if (cp && cp->isObject()) {
        const json::JsonValue* t = cp->find("title");
        const json::JsonValue* c = cp->find("creator");
        const json::JsonValue* cr = cp->find("created");
        const json::JsonValue* md = cp->find("modified");
        if (t && t->isString()) writeCore_.title = t->asString();
        if (c && c->isString()) writeCore_.creator = c->asString();
        if (cr && cr->isString()) writeCore_.created = cr->asString();
        if (md && md->isString()) writeCore_.modified = md->asString();
    }

    return true;
}

bool PptxConverter::writePptx_(const std::string& outputPath) {
    ooxml::ZipWriter zip;
    if (!zip.open(outputPath)) {
        err_ = "failed to open output file: " + outputPath + " (" + zip.error() + ")";
        return false;
    }

    const std::size_t slideCount = writeSlides_.size();
    const bool hasCoreProps = !writeCore_.title.empty() || !writeCore_.creator.empty() ||
                               !writeCore_.created.empty() || !writeCore_.modified.empty();

    // 1. [Content_Types].xml
    if (!zip.addFile("[Content_Types].xml",
                     ooxml::pml::PmlWriter::writeContentTypes(slideCount, hasCoreProps))) {
        err_ = zip.error();
        return false;
    }

    // 2. _rels/.rels
    if (!zip.addFile("_rels/.rels",
                     ooxml::pml::PmlWriter::writeRootRels())) {
        err_ = zip.error();
        return false;
    }

    // 3. ppt/presentation.xml
    if (!zip.addFile("ppt/presentation.xml",
                     ooxml::pml::PmlWriter::writePresentation(slideCount, writeSlideSize_))) {
        err_ = zip.error();
        return false;
    }

    // 4. ppt/_rels/presentation.xml.rels
    if (!zip.addFile("ppt/_rels/presentation.xml.rels",
                     ooxml::pml::PmlWriter::writePresentationRels(slideCount))) {
        err_ = zip.error();
        return false;
    }

    // 5. ppt/slides/slideN.xml
    for (std::size_t i = 0; i < slideCount; ++i) {
        std::string name = "ppt/slides/slide";
        name += std::to_string(i + 1);
        name += ".xml";
        if (!zip.addFile(name,
                         ooxml::pml::PmlWriter::writeSlide(writeSlides_[i]))) {
            err_ = zip.error();
            return false;
        }
    }

    // 6. ppt/slideLayouts/slideLayout1.xml
    if (!zip.addFile("ppt/slideLayouts/slideLayout1.xml",
                     ooxml::pml::PmlWriter::writeSlideLayout())) {
        err_ = zip.error();
        return false;
    }

    // 7. ppt/slideMasters/slideMaster1.xml
    if (!zip.addFile("ppt/slideMasters/slideMaster1.xml",
                     ooxml::pml::PmlWriter::writeSlideMaster())) {
        err_ = zip.error();
        return false;
    }

    // 8. ppt/_rels/slideMasters/slideMaster1.xml.rels
    if (!zip.addFile("ppt/_rels/slideMasters/slideMaster1.xml.rels",
                     ooxml::pml::PmlWriter::writeSlideMasterRels())) {
        err_ = zip.error();
        return false;
    }

    // 9. ppt/theme/theme1.xml
    if (!zip.addFile("ppt/theme/theme1.xml",
                     ooxml::pml::PmlWriter::writeTheme())) {
        err_ = zip.error();
        return false;
    }

    // 10. docProps/core.xml（可选）
    if (hasCoreProps) {
        if (!zip.addFile("docProps/core.xml",
                         ooxml::pml::PmlWriter::writeCoreProperties(writeCore_))) {
            err_ = zip.error();
            return false;
        }
    }

    if (!zip.close()) {
        err_ = zip.error();
        return false;
    }

    return true;
}

// ===========================================================================
// 错误 JSON 构造
// ===========================================================================

std::string PptxConverter::errorJson_(ErrorCode code, const std::string& message) {
    json::JsonValue result = json::JsonValue::object();
    result.set("code", static_cast<std::int64_t>(code));
    result.set("message", message);
    result.set("data", std::string(""));
    result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
    return json::serialize(result);
}

} // namespace converters
} // namespace office
} // namespace zq
