// =============================================================================
// src/api/converters/docx_converter.cpp
//
// DocxConverter 实现：
//   - 读取管线（H8-a）：docx → ZQ Doc JSON
//   - 写入管线（H8-b）：ZQ Doc JSON → docx
// =============================================================================
#include "docx_converter.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <utility>

#include "zq/office/version.h"
#include "ooxml/wml/wml_reader.h"
#include "ooxml/wml/wml_writer.h"
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

/// 检测 Heading 样式名 → 返回 level（1-9），非 Heading 返回 0
int detectHeadingLevel_(const std::string& style) {
    // 常见命名：Heading1 / heading 1 / heading1 / 标题 1
    if (style.empty()) return 0;
    // 大小写不敏感检查 "heading" 前缀
    std::string lower;
    lower.reserve(style.size());
    for (char c : style) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    // 查找 "heading" 子串
    std::string::size_type pos = lower.find("heading");
    if (pos == std::string::npos) return 0;
    pos += 7;  // skip "heading"
    // 跳过空格
    while (pos < lower.size() && (lower[pos] == ' ' || lower[pos] == '_')) ++pos;
    if (pos >= lower.size()) return 0;
    int lvl = static_cast<int>(lower[pos] - '0');
    if (lvl >= 1 && lvl <= 9) return lvl;
    return 0;
}

} // namespace

// ===========================================================================
// 构造 / 析构
// ===========================================================================

DocxConverter::DocxConverter() = default;
DocxConverter::~DocxConverter() = default;

// ===========================================================================
// 公开 API：读取（docx → JSON）
// ===========================================================================

std::string DocxConverter::toJson(const std::string& filePath,
                                  const std::string& dataDirectory,
                                  const std::string& optionsJson) {
    (void)dataDirectory;
    (void)optionsJson;

    if (filePath.empty()) {
        return errorJson_(ErrorCode::FileNotFound, "filePath is empty");
    }

    if (!loadArchive_(filePath)) {
        return errorJson_(ErrorCode::FileNotFound,
                          err_.empty() ? "failed to open docx" : err_);
    }

    if (!parseDocument_()) {
        return errorJson_(ErrorCode::XmlParseError,
                          err_.empty() ? "failed to parse document.xml" : err_);
    }

    // 样式表可选
    parseStyles_();

    // 关系可选
    parseDocumentRels_();

    // 核心属性可选
    parseCoreProperties_();

    // 组装 JSON
    json::JsonValue root = json::JsonValue::object();
    root.set("blocks", buildBlocksJson_());

    // coreProperties
    json::JsonValue coreObj = json::JsonValue::object();
    coreObj.set("title", read_.title);
    coreObj.set("creator", read_.creator);
    coreObj.set("created", read_.created);
    coreObj.set("modified", read_.modified);
    root.set("coreProperties", std::move(coreObj));

    // 包装为 ExportResult
    json::JsonValue exportResult = json::JsonValue::object();
    exportResult.set("code", static_cast<std::int64_t>(ErrorCode::OK));
    exportResult.set("message", std::string("ok"));
    exportResult.set("data", json::serialize(root));
    exportResult.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
    return json::serialize(exportResult);
}

// ===========================================================================
// 公开 API：写入（JSON → docx）
// ===========================================================================

std::string DocxConverter::fromJson(const std::string& clientVarsJson,
                                    const std::string& outputPath,
                                    const std::string& dataDirectory,
                                    const std::string& optionsJson) {
    (void)dataDirectory;
    (void)optionsJson;

    if (clientVarsJson.empty()) {
        return errorJson_(ErrorCode::InvalidArgument, "clientVarsJson is empty");
    }
    if (outputPath.empty()) {
        return errorJson_(ErrorCode::InvalidArgument, "outputPath is empty");
    }

    if (!parseZQDocJson_(clientVarsJson)) {
        return errorJson_(ErrorCode::ConversionError,
                          err_.empty() ? "failed to parse JSON" : err_);
    }

    if (!writeDocx_(outputPath)) {
        return errorJson_(ErrorCode::ZipError,
                          err_.empty() ? "failed to write docx" : err_);
    }

    // 成功
    json::JsonValue exportResult = json::JsonValue::object();
    exportResult.set("code", static_cast<std::int64_t>(ErrorCode::OK));
    exportResult.set("message", std::string("ok"));
    exportResult.set("data", std::string(""));
    exportResult.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
    return json::serialize(exportResult);
}

// ===========================================================================
// 内部读取辅助
// ===========================================================================

bool DocxConverter::loadArchive_(const std::string& filePath) {
    zip_ = std::make_unique<ooxml::ZipReader>();
    if (!zip_->open(filePath)) {
        err_ = "failed to open zip: " + filePath;
        return false;
    }
    return true;
}

bool DocxConverter::parseDocument_() {
    std::string docXml = zip_->readEntryText("word/document.xml");
    if (docXml.empty()) {
        err_ = "word/document.xml not found or empty";
        return false;
    }

    ooxml::wml::WmlReader reader;
    if (!reader.parseDocument(docXml)) {
        err_ = "failed to parse document.xml: " + reader.error();
        return false;
    }

    read_.blocks = reader.blocks();  // 拷贝
    return true;
}

bool DocxConverter::parseStyles_() {
    std::string stylesXml = zip_->readEntryText("word/styles.xml");
    if (stylesXml.empty()) {
        return false;
    }

    ooxml::wml::WmlReader reader;
    if (!reader.parseStyles(stylesXml)) {
        return false;
    }
    read_.styleNames = reader.styleNames();
    return true;
}

bool DocxConverter::parseDocumentRels_() {
    std::string relsXml = zip_->readEntryText("word/_rels/document.xml.rels");
    if (relsXml.empty()) {
        return false;
    }

    read_.rels.clear();
    std::string::size_type pos = 0;
    while (true) {
        std::string::size_type relPos = relsXml.find("Relationship", pos);
        if (relPos == std::string::npos) break;

        std::string id;
        std::string target;

        std::string::size_type idPos = relsXml.find("Id=\"", relPos);
        if (idPos != std::string::npos) {
            idPos += 4;
            std::string::size_type idEnd = relsXml.find('"', idPos);
            if (idEnd != std::string::npos) {
                id = relsXml.substr(idPos, idEnd - idPos);
            }
        }

        std::string::size_type tgtPos = relsXml.find("Target=\"", relPos);
        if (tgtPos != std::string::npos) {
            tgtPos += 8;
            std::string::size_type tgtEnd = relsXml.find('"', tgtPos);
            if (tgtEnd != std::string::npos) {
                target = relsXml.substr(tgtPos, tgtEnd - tgtPos);
            }
        }

        if (!id.empty()) {
            read_.rels.emplace_back(std::move(id), std::move(target));
        }

        pos = relPos + 12;
    }
    return true;
}

bool DocxConverter::parseCoreProperties_() {
    std::string coreXml = zip_->readEntryText("docProps/core.xml");
    if (coreXml.empty()) {
        return false;
    }

    // 简单字符串提取
    auto extract_ = [&](const std::string& tag) -> std::string {
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

    read_.title = extract_("dc:title");
    read_.creator = extract_("dc:creator");
    read_.created = extract_("dcterms:created");
    read_.modified = extract_("dcterms:modified");
    return true;
}

json::JsonValue DocxConverter::buildBlocksJson_() {
    json::JsonValue blocksArr = json::JsonValue::array();
    int blockIndex = 0;

    for (const auto& block : read_.blocks) {
        json::JsonValue blockObj = json::JsonValue::object();

        std::string blockId = "block_";
        blockId += std::to_string(blockIndex++);
        blockObj.set("id", blockId);

        switch (block.type) {
            case ooxml::wml::WmlBlockType::Paragraph: {
                // 检测是否为 Heading
                int headingLvl = detectHeadingLevel_(block.paragraph.style);
                if (headingLvl > 0) {
                    blockObj.set("type", std::string("heading"));
                    json::JsonValue headingObj = json::JsonValue::object();
                    headingObj.set("level", static_cast<std::int64_t>(headingLvl));
                    // 拼接所有 run 的文本
                    std::string text;
                    for (const auto& run : block.paragraph.runs) {
                        text += run.text;
                    }
                    headingObj.set("text", text);
                    blockObj.set("heading", std::move(headingObj));
                } else {
                    blockObj.set("type", std::string("paragraph"));
                    json::JsonValue paraObj = json::JsonValue::object();
                    paraObj.set("style", block.paragraph.style);
                    // alignment 映射
                    std::string algn = block.paragraph.alignment;
                    if (algn == "center") {
                        paraObj.set("alignment", std::string("center"));
                    } else if (algn == "right") {
                        paraObj.set("alignment", std::string("right"));
                    } else if (algn == "both" || algn == "justify") {
                        paraObj.set("alignment", std::string("justify"));
                    } else {
                        paraObj.set("alignment", std::string("left"));
                    }
                    // runs 数组
                    json::JsonValue runsArr = json::JsonValue::array();
                    for (const auto& run : block.paragraph.runs) {
                        json::JsonValue runObj = json::JsonValue::object();
                        runObj.set("text", run.text);
                        json::JsonValue fontObj = json::JsonValue::object();
                        if (!run.font.family.empty()) {
                            fontObj.set("family", run.font.family);
                        }
                        if (run.font.size > 0) {
                            // 半磅 → 磅
                            fontObj.set("size",
                                        static_cast<std::int64_t>(run.font.size / 2));
                        }
                        if (run.font.bold)     fontObj.set("bold", true);
                        if (run.font.italic)   fontObj.set("italic", true);
                        if (run.font.underline) fontObj.set("underline", true);
                        if (!run.font.color.empty()) {
                            fontObj.set("color", run.font.color);
                        }
                        runObj.set("font", std::move(fontObj));
                        runsArr.append(std::move(runObj));
                    }
                    paraObj.set("runs", std::move(runsArr));
                    blockObj.set("paragraph", std::move(paraObj));
                }
                break;
            }
            case ooxml::wml::WmlBlockType::Table: {
                blockObj.set("type", std::string("table"));
                json::JsonValue tableObj = json::JsonValue::object();
                tableObj.set("rows", static_cast<std::int64_t>(block.table.rows));
                tableObj.set("columns",
                             static_cast<std::int64_t>(block.table.columns));
                json::JsonValue cellsArr = json::JsonValue::array();
                for (const auto& cell : block.table.cells) {
                    json::JsonValue cellObj = json::JsonValue::object();
                    cellObj.set("row", static_cast<std::int64_t>(cell.row));
                    cellObj.set("column", static_cast<std::int64_t>(cell.column));
                    cellObj.set("text", cell.text);
                    cellsArr.append(std::move(cellObj));
                }
                tableObj.set("cells", std::move(cellsArr));
                blockObj.set("table", std::move(tableObj));
                break;
            }
            case ooxml::wml::WmlBlockType::Image: {
                blockObj.set("type", std::string("image"));
                json::JsonValue imageObj = json::JsonValue::object();
                // 查找 relId 对应的 media 路径
                std::string mediaPath;
                for (const auto& rel : read_.rels) {
                    if (rel.first == block.image.relId) {
                        mediaPath = rel.second;
                        break;
                    }
                }
                // relId 作为 path（若未找到关系，则用 relId）
                imageObj.set("path",
                             mediaPath.empty() ? block.image.relId : mediaPath);
                imageObj.set("width",
                             static_cast<std::int64_t>(block.image.width));
                imageObj.set("height",
                             static_cast<std::int64_t>(block.image.height));
                blockObj.set("image", std::move(imageObj));
                break;
            }
        }

        blocksArr.append(std::move(blockObj));
    }

    return blocksArr;
}

// ===========================================================================
// 内部写入辅助
// ===========================================================================

bool DocxConverter::parseZQDocJson_(const std::string& clientVarsJson) {
    writeBlocks_.clear();
    writeCore_ = ooxml::wml::WriteCoreProperties{};

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

    // blocks 数组（必需）
    const json::JsonValue* blocksArr = dataPtr->find("blocks");
    if (!blocksArr || !blocksArr->isArray()) {
        err_ = "missing 'blocks' array in clientVars JSON";
        return false;
    }

    for (std::size_t i = 0; i < blocksArr->size(); ++i) {
        const json::JsonValue& blockJson = blocksArr->at(i);
        if (!blockJson.isObject()) {
            err_ = "block[";
            err_ += std::to_string(i);
            err_ += "] is not an object";
            return false;
        }

        ooxml::wml::WriteBlock block;
        const json::JsonValue* typeField = blockJson.find("type");
        std::string typeStr = (typeField && typeField->isString())
                                  ? typeField->asString() : "paragraph";

        if (typeStr == "paragraph") {
            block.type = ooxml::wml::WriteBlockType::Paragraph;
            const json::JsonValue* paraField = blockJson.find("paragraph");
            if (paraField && paraField->isObject()) {
                // alignment
                const json::JsonValue* algn = paraField->find("alignment");
                if (algn && algn->isString()) {
                    const std::string& a = algn->asString();
                    if (a == "center") {
                        block.paragraph.alignment = ooxml::wml::WriteAlignment::Center;
                    } else if (a == "right") {
                        block.paragraph.alignment = ooxml::wml::WriteAlignment::Right;
                    } else if (a == "justify") {
                        block.paragraph.alignment = ooxml::wml::WriteAlignment::Justify;
                    } else {
                        block.paragraph.alignment = ooxml::wml::WriteAlignment::Left;
                    }
                }
                // runs
                const json::JsonValue* runsArr = paraField->find("runs");
                if (runsArr && runsArr->isArray()) {
                    for (std::size_t j = 0; j < runsArr->size(); ++j) {
                        const json::JsonValue& runJson = runsArr->at(j);
                        if (!runJson.isObject()) continue;
                        ooxml::wml::WriteRun run;
                        const json::JsonValue* txt = runJson.find("text");
                        if (txt && txt->isString()) run.text = txt->asString();
                        const json::JsonValue* fnt = runJson.find("font");
                        if (fnt && fnt->isObject()) {
                            const json::JsonValue* fam = fnt->find("family");
                            if (fam && fam->isString()) run.font.family = fam->asString();
                            const json::JsonValue* sz = fnt->find("size");
                            if (sz && sz->isNumber()) {
                                // 磅 → 半磅
                                run.font.size = static_cast<std::int32_t>(sz->asInt() * 2);
                            }
                            const json::JsonValue* b = fnt->find("bold");
                            if (b && b->isBool()) run.font.bold = b->asBool();
                            const json::JsonValue* it = fnt->find("italic");
                            if (it && it->isBool()) run.font.italic = it->asBool();
                            const json::JsonValue* u = fnt->find("underline");
                            if (u && u->isBool()) run.font.underline = u->asBool();
                            const json::JsonValue* clr = fnt->find("color");
                            if (clr && clr->isString()) run.font.color = clr->asString();
                        }
                        block.paragraph.runs.push_back(std::move(run));
                    }
                }
            }
        } else if (typeStr == "heading") {
            block.type = ooxml::wml::WriteBlockType::Heading;
            const json::JsonValue* hField = blockJson.find("heading");
            if (hField && hField->isObject()) {
                const json::JsonValue* lvl = hField->find("level");
                if (lvl && lvl->isNumber()) {
                    block.headingLevel = static_cast<std::int32_t>(lvl->asInt());
                }
                const json::JsonValue* txt = hField->find("text");
                if (txt && txt->isString()) {
                    ooxml::wml::WriteRun run;
                    run.text = txt->asString();
                    block.paragraph.runs.push_back(std::move(run));
                }
            }
        } else if (typeStr == "table") {
            block.type = ooxml::wml::WriteBlockType::Table;
            const json::JsonValue* tField = blockJson.find("table");
            if (tField && tField->isObject()) {
                const json::JsonValue* rows = tField->find("rows");
                const json::JsonValue* cols = tField->find("columns");
                if (rows && rows->isNumber())
                    block.table.rows = static_cast<std::int32_t>(rows->asInt());
                if (cols && cols->isNumber())
                    block.table.columns = static_cast<std::int32_t>(cols->asInt());
                const json::JsonValue* cellsArr = tField->find("cells");
                if (cellsArr && cellsArr->isArray()) {
                    for (std::size_t j = 0; j < cellsArr->size(); ++j) {
                        const json::JsonValue& cellJson = cellsArr->at(j);
                        if (!cellJson.isObject()) continue;
                        ooxml::wml::WriteTableCell cell;
                        const json::JsonValue* r = cellJson.find("row");
                        const json::JsonValue* c = cellJson.find("column");
                        const json::JsonValue* t = cellJson.find("text");
                        if (r && r->isNumber())
                            cell.row = static_cast<std::int32_t>(r->asInt());
                        if (c && c->isNumber())
                            cell.column = static_cast<std::int32_t>(c->asInt());
                        if (t && t->isString()) cell.text = t->asString();
                        block.table.cells.push_back(std::move(cell));
                    }
                }
            }
        } else if (typeStr == "image") {
            block.type = ooxml::wml::WriteBlockType::Image;
            const json::JsonValue* iField = blockJson.find("image");
            if (iField && iField->isObject()) {
                const json::JsonValue* p = iField->find("path");
                const json::JsonValue* w = iField->find("width");
                const json::JsonValue* h = iField->find("height");
                if (p && p->isString()) block.image.relId = p->asString();
                if (w && w->isNumber())
                    block.image.width = w->asInt();
                if (h && h->isNumber())
                    block.image.height = h->asInt();
            }
        } else {
            // 未知类型，按段落处理
            block.type = ooxml::wml::WriteBlockType::Paragraph;
        }

        writeBlocks_.push_back(std::move(block));
    }

    // coreProperties（可选）
    const json::JsonValue* coreField = dataPtr->find("coreProperties");
    if (coreField && coreField->isObject()) {
        const json::JsonValue* t = coreField->find("title");
        const json::JsonValue* c = coreField->find("creator");
        const json::JsonValue* cr = coreField->find("created");
        const json::JsonValue* md = coreField->find("modified");
        if (t && t->isString()) writeCore_.title = t->asString();
        if (c && c->isString()) writeCore_.creator = c->asString();
        if (cr && cr->isString()) writeCore_.created = cr->asString();
        if (md && md->isString()) writeCore_.modified = md->asString();
    }

    return true;
}

bool DocxConverter::writeDocx_(const std::string& outputPath) {
    ooxml::ZipWriter zip;
    if (!zip.open(outputPath)) {
        err_ = "failed to open output file: " + outputPath + " (" + zip.error() + ")";
        return false;
    }

    const bool hasCoreProps = !writeCore_.title.empty() ||
                               !writeCore_.creator.empty() ||
                               !writeCore_.created.empty() ||
                               !writeCore_.modified.empty();

    // 统计图片数量
    std::size_t imageCount = 0;
    for (const auto& block : writeBlocks_) {
        if (block.type == ooxml::wml::WriteBlockType::Image) {
            ++imageCount;
        }
    }

    // 1. [Content_Types].xml
    if (!zip.addFile("[Content_Types].xml",
                     ooxml::wml::WmlWriter::writeContentTypes(hasCoreProps))) {
        err_ = zip.error();
        return false;
    }

    // 2. _rels/.rels
    if (!zip.addFile("_rels/.rels",
                     ooxml::wml::WmlWriter::writeRootRels())) {
        err_ = zip.error();
        return false;
    }

    // 3. word/document.xml
    if (!zip.addFile("word/document.xml",
                     ooxml::wml::WmlWriter::writeDocument(writeBlocks_))) {
        err_ = zip.error();
        return false;
    }

    // 4. word/_rels/document.xml.rels
    if (!zip.addFile("word/_rels/document.xml.rels",
                     ooxml::wml::WmlWriter::writeDocumentRels(imageCount))) {
        err_ = zip.error();
        return false;
    }

    // 5. word/styles.xml
    if (!zip.addFile("word/styles.xml",
                     ooxml::wml::WmlWriter::writeStyles())) {
        err_ = zip.error();
        return false;
    }

    // 6. docProps/core.xml（可选）
    if (hasCoreProps) {
        if (!zip.addFile("docProps/core.xml",
                         ooxml::wml::WmlWriter::writeCoreProperties(writeCore_))) {
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

std::string DocxConverter::errorJson_(ErrorCode code, const std::string& message) {
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
