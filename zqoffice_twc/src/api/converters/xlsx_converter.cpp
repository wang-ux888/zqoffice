// =============================================================================
// src/api/converters/xlsx_converter.cpp
//
// XlsxConverter 实现：xlsx → ZQ Sheet JSON（Phase H4）
// =============================================================================
#include "xlsx_converter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <utility>

#include "zq/office/version.h"
#include "numfmt/date_util.h"
#include "ooxml/sml/sml_reader.h"
#include "ooxml/sml/sml_writer.h"
#include "ooxml/zip_reader.h"
#include "ooxml/zip_writer.h"

namespace zq {
namespace office {
namespace converters {

// ===========================================================================
// 构造 / 析构
// ===========================================================================

XlsxConverter::XlsxConverter() = default;
XlsxConverter::~XlsxConverter() = default;

// ===========================================================================
// 公开 API
// ===========================================================================

std::string XlsxConverter::toJson(const std::string& filePath,
                                  const std::string& dataDirectory,
                                  const std::string& optionsJson) {
    (void)dataDirectory;    // Phase H4 暂不处理图片提取
    (void)optionsJson;      // Phase H4 暂不解析选项

    if (!loadArchive_(filePath)) {
        return errorJson_(ErrorCode::FileNotFound, err_.empty() ? "failed to open xlsx" : err_);
    }

    if (!parseWorkbook_()) {
        return errorJson_(ErrorCode::ZipError, "failed to parse workbook.xml");
    }

    if (!parseSharedStrings_()) {
        // sharedStrings.xml 可选，失败不致命
    }

    if (!parseStyles_()) {
        // styles.xml 可选，失败不致命
    }

    // 解析所有工作表
    sheetResults_.clear();
    sheetResults_.resize(workbook_.sheets.size());
    for (std::size_t i = 0; i < workbook_.sheets.size(); ++i) {
        if (!parseWorksheet_(static_cast<int>(i))) {
            // 单个 sheet 解析失败，记录但继续
            sheetResults_[i].dimension = "";
        }
    }

    // 组装最终 JSON
    json::JsonValue root = json::JsonValue::object();

    // sheets 数组
    json::JsonValue sheetsArr = json::JsonValue::array();
    for (std::size_t i = 0; i < workbook_.sheets.size(); ++i) {
        sheetsArr.append(buildSheetJson_(static_cast<int>(i)));
    }
    root.set("sheets", std::move(sheetsArr));

    // definedNames
    root.set("definedNames", buildDefinedNamesJson_());

    // coreProperties
    root.set("coreProperties", buildCorePropertiesJson_());

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

std::string XlsxConverter::fromJson(const std::string& clientVarsJson,
                                    const std::string& outputPath,
                                    const std::string& dataDirectory,
                                    const std::string& optionsJson) {
    (void)dataDirectory;    // Phase H5 暂不处理图片资源
    (void)optionsJson;      // Phase H5 暂不解析选项

    if (outputPath.empty()) {
        return errorJson_(ErrorCode::InvalidArgument, "outputPath is empty");
    }

    if (clientVarsJson.empty()) {
        return errorJson_(ErrorCode::InvalidArgument, "clientVarsJson is empty");
    }

    // 1. 解析 ZQ Sheet JSON → 内部文档模型
    if (!parseZQSheetJson_(clientVarsJson)) {
        return errorJson_(ErrorCode::ConversionError,
                          err_.empty() ? "failed to parse clientVars JSON" : err_);
    }

    // 2. 收集所有字符串单元格 → sharedStrings 表
    collectSharedStrings_();

    // 3. 生成 XML 部件并用 ZipWriter 打包
    if (!writeXlsx_(outputPath)) {
        return errorJson_(ErrorCode::ZipError,
                          err_.empty() ? "failed to write xlsx" : err_);
    }

    // 4. 返回成功结果
    json::JsonValue result = json::JsonValue::object();
    result.set("code", static_cast<std::int64_t>(ErrorCode::OK));
    result.set("message", std::string("ok"));
    result.set("data", std::string(""));
    result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
    return json::serialize(result);
}

// ===========================================================================
// 内部写入辅助（Phase H5）
// ===========================================================================

bool XlsxConverter::parseZQSheetJson_(const std::string& clientVarsJson) {
    writeSheets_.clear();
    writeNames_.clear();
    writeCore_ = ooxml::sml::WriteCoreProperties{};

    // 解析 JSON：可能是 ExportResult 包装（{code,message,data}），也可能是直接 data
    json::JsonValue root;
    try {
        root = json::parse(clientVarsJson);
    } catch (const std::exception& e) {
        err_ = "JSON parse error: ";
        err_ += e.what();
        return false;
    }

    // 检测是否为 ExportResult 包装
    const json::JsonValue* dataPtr = &root;
    json::JsonValue innerData;
    if (root.isObject() && root.contains("code") && root.contains("data")) {
        const json::JsonValue* dataField = root.find("data");
        if (dataField && dataField->isString()) {
            // data 是字符串，需要二次解析
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

    // 解析 sheets 数组
    const json::JsonValue* sheetsArr = dataPtr->find("sheets");
    if (!sheetsArr || !sheetsArr->isArray()) {
        err_ = "missing 'sheets' array in clientVars JSON";
        return false;
    }

    for (std::size_t i = 0; i < sheetsArr->size(); ++i) {
        const json::JsonValue& sheetJson = sheetsArr->at(i);
        if (!sheetJson.isObject()) {
            err_ = "sheet[";
            err_ += std::to_string(i);
            err_ += "] is not an object";
            return false;
        }

        WriteSheetMeta meta;
        const json::JsonValue* name = sheetJson.find("name");
        if (name && name->isString()) {
            meta.name = name->asString();
        } else {
            meta.name = std::string("Sheet") + std::to_string(i + 1);
        }

        const json::JsonValue* sid = sheetJson.find("sheetId");
        if (sid && sid->isString()) {
            meta.sheetId = sid->asString();
        } else {
            meta.sheetId = std::string("sheet_") + std::to_string(i);
        }

        if (!buildWriteSheetData_(sheetJson, meta.data)) {
            return false;
        }

        writeSheets_.push_back(std::move(meta));
    }

    // 解析 definedNames（可选）
    const json::JsonValue* dnArr = dataPtr->find("definedNames");
    if (dnArr && dnArr->isArray()) {
        for (std::size_t i = 0; i < dnArr->size(); ++i) {
            const json::JsonValue& dn = dnArr->at(i);
            if (!dn.isObject()) continue;
            ooxml::sml::WriteDefinedName wdn;
            const json::JsonValue* nm = dn.find("name");
            const json::JsonValue* rf = dn.find("ref");
            if (nm && nm->isString()) wdn.name = nm->asString();
            if (rf && rf->isString()) wdn.ref = rf->asString();
            if (!wdn.name.empty()) {
                writeNames_.push_back(std::move(wdn));
            }
        }
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

bool XlsxConverter::buildWriteSheetData_(const json::JsonValue& sheetJson,
                                          ooxml::sml::WriteSheetData& out) {
    out.rows.clear();
    out.merges.clear();

    // 解析 cells 数组 → 按 row 分组
    const json::JsonValue* cellsArr = sheetJson.find("cells");
    if (cellsArr && cellsArr->isArray()) {
        // 用 map 按 row 分组（row → cells）
        std::map<std::uint32_t, std::vector<ooxml::sml::WriteCell>> rowMap;

        for (std::size_t i = 0; i < cellsArr->size(); ++i) {
            const json::JsonValue& cellJson = cellsArr->at(i);
            if (!cellJson.isObject()) continue;

            const json::JsonValue* r = cellJson.find("row");
            const json::JsonValue* c = cellJson.find("column");
            const json::JsonValue* t = cellJson.find("type");
            const json::JsonValue* v = cellJson.find("value");

            if (!r || !c || !r->isNumber() || !c->isNumber()) continue;

            ooxml::sml::WriteCell cell;
            cell.row = static_cast<std::uint32_t>(r->asInt()) + 1;  // JSON 0-based → 1-based
            cell.col = static_cast<std::uint32_t>(c->asInt()) + 1;

            // 默认数字类型
            cell.type = ooxml::sml::WriteCellType::Number;

            std::string typeStr;
            if (t && t->isString()) {
                typeStr = t->asString();
            }

            if (typeStr == "string") {
                cell.type = ooxml::sml::WriteCellType::SharedString;
                if (v && v->isString()) {
                    cell.value = v->asString();
                } else if (v) {
                    // 非字符串值转字符串
                    if (v->isBool()) {
                        cell.value = v->asBool() ? "true" : "false";
                    } else if (v->isInt()) {
                        cell.value = std::to_string(v->asInt());
                    } else if (v->isDouble()) {
                        cell.value = std::to_string(v->asDouble());
                    }
                }
            } else if (typeStr == "boolean") {
                cell.type = ooxml::sml::WriteCellType::Boolean;
                if (v && v->isBool()) {
                    cell.value = v->asBool() ? "1" : "0";
                } else if (v && v->isInt()) {
                    cell.value = v->asInt() != 0 ? "1" : "0";
                } else {
                    cell.value = "0";
                }
            } else if (typeStr == "error") {
                cell.type = ooxml::sml::WriteCellType::Error;
                if (v && v->isString()) cell.value = v->asString();
            } else {
                // number（默认）
                cell.type = ooxml::sml::WriteCellType::Number;
                if (v) {
                    if (v->isInt()) {
                        cell.value = std::to_string(v->asInt());
                    } else if (v->isDouble()) {
                        // 用最短 round-trip 表示（复用 json serializer 的逻辑）
                        char buf[64];
                        double d = v->asDouble();
                        if (d == static_cast<double>(static_cast<std::int64_t>(d)) &&
                            std::abs(d) < 1e15) {
                            std::snprintf(buf, sizeof(buf), "%.1f", d);
                        } else {
                            for (int prec = 1; prec <= 17; ++prec) {
                                std::snprintf(buf, sizeof(buf), "%.*g", prec, d);
                                if (std::strtod(buf, nullptr) == d) break;
                            }
                        }
                        cell.value = buf;
                    } else if (v->isString()) {
                        cell.value = v->asString();
                    }
                }
            }

            rowMap[cell.row].push_back(cell);
        }

        // 转为 WriteRow 数组（按 row 排序）
        for (auto& kv : rowMap) {
            ooxml::sml::WriteRow row;
            row.row = kv.first;
            row.cells = std::move(kv.second);
            out.rows.push_back(std::move(row));
        }
    }

    // 解析 mergedCells 数组
    const json::JsonValue* mcArr = sheetJson.find("mergedCells");
    if (mcArr && mcArr->isArray()) {
        for (std::size_t i = 0; i < mcArr->size(); ++i) {
            const json::JsonValue& mc = mcArr->at(i);
            if (!mc.isObject()) continue;

            ooxml::sml::WriteMergeRange m;
            const json::JsonValue* sr = mc.find("startRow");
            const json::JsonValue* sc = mc.find("startColumn");
            const json::JsonValue* er = mc.find("endRow");
            const json::JsonValue* ec = mc.find("endColumn");

            if (sr && sr->isNumber()) m.startRow = static_cast<std::uint32_t>(sr->asInt()) + 1;
            if (sc && sc->isNumber()) m.startCol = static_cast<std::uint32_t>(sc->asInt()) + 1;
            if (er && er->isNumber()) m.endRow = static_cast<std::uint32_t>(er->asInt()) + 1;
            if (ec && ec->isNumber()) m.endCol = static_cast<std::uint32_t>(ec->asInt()) + 1;

            if (m.startRow && m.startCol && m.endRow && m.endCol) {
                out.merges.push_back(m);
            }
        }
    }

    return true;
}

void XlsxConverter::collectSharedStrings_() {
    writeSharedStrings_.clear();
    writeStringIndex_.clear();

    // 遍历所有 sheet 的所有单元格，收集 SharedString 类型的值
    for (auto& sheet : writeSheets_) {
        for (auto& row : sheet.data.rows) {
            for (auto& cell : row.cells) {
                if (cell.type == ooxml::sml::WriteCellType::SharedString) {
                    auto it = writeStringIndex_.find(cell.value);
                    if (it == writeStringIndex_.end()) {
                        std::size_t idx = writeSharedStrings_.size();
                        writeSharedStrings_.push_back(cell.value);
                        writeStringIndex_[cell.value] = idx;
                        // 把 cell.value 替换为索引字符串（供 SmlWriter 写入 <v>）
                        cell.value = std::to_string(idx);
                    } else {
                        cell.value = std::to_string(it->second);
                    }
                }
            }
        }
    }
}

bool XlsxConverter::writeXlsx_(const std::string& outputPath) {
    ooxml::ZipWriter zip;
    if (!zip.open(outputPath)) {
        err_ = "failed to open output file: " + outputPath + " (" + zip.error() + ")";
        return false;
    }

    const std::size_t sheetCount = writeSheets_.size();
    const bool hasSharedStrings = !writeSharedStrings_.empty();
    const bool hasCoreProps = !writeCore_.title.empty() || !writeCore_.creator.empty() ||
                               !writeCore_.created.empty() || !writeCore_.modified.empty();

    // 1. [Content_Types].xml
    if (!zip.addFile("[Content_Types].xml",
                     ooxml::sml::SmlWriter::writeContentTypes(sheetCount, hasSharedStrings))) {
        err_ = zip.error();
        return false;
    }

    // 2. _rels/.rels
    if (!zip.addFile("_rels/.rels",
                     ooxml::sml::SmlWriter::writeRootRels(hasCoreProps))) {
        err_ = zip.error();
        return false;
    }

    // 3. xl/workbook.xml
    std::vector<ooxml::sml::WriteSheetInfo> sheetInfos;
    sheetInfos.reserve(sheetCount);
    for (std::size_t i = 0; i < sheetCount; ++i) {
        ooxml::sml::WriteSheetInfo info;
        info.name = writeSheets_[i].name;
        info.sheetId = static_cast<std::uint32_t>(i + 1);
        info.relId = std::string("rId") + std::to_string(i + 1);
        sheetInfos.push_back(std::move(info));
    }
    if (!zip.addFile("xl/workbook.xml",
                     ooxml::sml::SmlWriter::writeWorkbook(sheetInfos, writeNames_))) {
        err_ = zip.error();
        return false;
    }

    // 4. xl/_rels/workbook.xml.rels
    if (!zip.addFile("xl/_rels/workbook.xml.rels",
                     ooxml::sml::SmlWriter::writeWorkbookRels(sheetCount, hasSharedStrings, true))) {
        err_ = zip.error();
        return false;
    }

    // 5. xl/worksheets/sheetN.xml
    for (std::size_t i = 0; i < sheetCount; ++i) {
        std::string name = "xl/worksheets/sheet";
        name += std::to_string(i + 1);
        name += ".xml";
        if (!zip.addFile(name,
                         ooxml::sml::SmlWriter::writeWorksheet(writeSheets_[i].data))) {
            err_ = zip.error();
            return false;
        }
    }

    // 6. xl/sharedStrings.xml（可选）
    if (hasSharedStrings) {
        if (!zip.addFile("xl/sharedStrings.xml",
                         ooxml::sml::SmlWriter::writeSharedStrings(writeSharedStrings_))) {
            err_ = zip.error();
            return false;
        }
    }

    // 7. xl/styles.xml（始终写入最小样式）
    if (!zip.addFile("xl/styles.xml", ooxml::sml::SmlWriter::writeStyles())) {
        err_ = zip.error();
        return false;
    }

    // 8. docProps/core.xml（可选）
    if (hasCoreProps) {
        if (!zip.addFile("docProps/core.xml",
                         ooxml::sml::SmlWriter::writeCoreProperties(writeCore_))) {
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
// 内部读取辅助
// ===========================================================================

bool XlsxConverter::loadArchive_(const std::string& filePath) {
    zip_ = std::make_unique<ooxml::ZipReader>();
    if (!zip_->open(filePath)) {
        err_ = "failed to open zip: " + filePath;
        return false;
    }
    smlReader_ = std::make_unique<ooxml::sml::SmlReader>();
    return true;
}

bool XlsxConverter::parseWorkbook_() {
    // 1. 读取 xl/workbook.xml
    std::string wbXml = zip_->readEntryText("xl/workbook.xml");
    if (wbXml.empty()) {
        err_ = "xl/workbook.xml not found or empty";
        return false;
    }

    if (!smlReader_->parseWorkbook(wbXml)) {
        err_ = "failed to parse workbook.xml: " + smlReader_->error();
        return false;
    }

    workbook_.sheets = smlReader_->sheets();

    // 2. 读取 xl/_rels/workbook.xml.rels 获取 r:id → 文件路径映射
    std::string relsXml = zip_->readEntryText("xl/_rels/workbook.xml.rels");
    if (!relsXml.empty()) {
        // 简单解析 <Relationship Id="..." Target="worksheets/sheet1.xml" .../>
        // 使用 pugixml 或简单字符串查找
        // 这里用简单字符串查找（避免引入 XmlReader 依赖）
        std::string::size_type pos = 0;
        while (true) {
            std::string::size_type relPos = relsXml.find("Relationship", pos);
            if (relPos == std::string::npos) break;
            // 查找 Id
            std::string id;
            std::string::size_type idPos = relsXml.find("Id=\"", relPos);
            if (idPos != std::string::npos) {
                idPos += 4;
                std::string::size_type idEnd = relsXml.find('"', idPos);
                if (idEnd != std::string::npos) {
                    id = relsXml.substr(idPos, idEnd - idPos);
                }
            }
            // 查找 Target
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
                // Target 是相对路径（如 "worksheets/sheet1.xml"），需转为 "xl/worksheets/sheet1.xml"
                std::string fullPath = target;
                if (fullPath.find("xl/") != 0 && fullPath.find('/') != 0) {
                    fullPath = "xl/" + fullPath;
                }
                workbook_.rels.emplace_back(id, fullPath);
            }
            pos = relPos + 12;
        }
    }

    // 3. 解析 definedNames（简单字符串查找）
    // 注意：搜索 "<definedName "（带空格）以避免匹配容器元素 <definedNames>
    std::string::size_type dnPos = wbXml.find("<definedNames>");
    if (dnPos != std::string::npos) {
        std::string::size_type dnEnd = wbXml.find("</definedNames>", dnPos);
        if (dnEnd != std::string::npos) {
            std::string dnBlock = wbXml.substr(dnPos, dnEnd - dnPos);
            // 查找 <definedName name="...">...</definedName>
            // 用 "<definedName " 带空格避免匹配 <definedNames>
            std::string::size_type searchPos = 0;
            while (true) {
                std::string::size_type namePos = dnBlock.find("<definedName ", searchPos);
                if (namePos == std::string::npos) break;
                // 提取 name 属性
                std::string name;
                std::string::size_type attrStart = dnBlock.find("name=\"", namePos);
                if (attrStart != std::string::npos) {
                    attrStart += 6;
                    std::string::size_type attrEnd = dnBlock.find('"', attrStart);
                    if (attrEnd != std::string::npos) {
                        name = dnBlock.substr(attrStart, attrEnd - attrStart);
                    }
                }
                // 提取值（>...</definedName>）
                // 注意：从 name 属性后开始找 '>'，避免匹配属性值中的 '>'
                std::string::size_type valStart = dnBlock.find('>', attrStart);
                std::string::size_type valEnd = dnBlock.find("</definedName>", valStart);
                if (valStart != std::string::npos && valEnd != std::string::npos) {
                    std::string value = dnBlock.substr(valStart + 1, valEnd - valStart - 1);
                    workbook_.definedNames.emplace_back(name, value);
                }
                searchPos = valEnd + 14;
            }
        }
    }

    return true;
}

bool XlsxConverter::parseSharedStrings_() {
    if (!zip_->hasEntry("xl/sharedStrings.xml")) {
        return false;  // 可选文件
    }
    std::string sstXml = zip_->readEntryText("xl/sharedStrings.xml");
    if (sstXml.empty()) {
        return false;
    }
    if (!smlReader_->parseSharedStrings(sstXml)) {
        return false;
    }
    sharedStrings_ = smlReader_->sharedStrings();
    return true;
}

bool XlsxConverter::parseStyles_() {
    if (!zip_->hasEntry("xl/styles.xml")) {
        return false;
    }
    std::string stylesXml = zip_->readEntryText("xl/styles.xml");
    if (stylesXml.empty()) {
        return false;
    }
    // 简单解析 <numFmt numFmtId="..." formatCode="..."/>
    std::string::size_type pos = 0;
    while (true) {
        std::string::size_type fmtPos = stylesXml.find("<numFmt", pos);
        if (fmtPos == std::string::npos) break;
        // numFmtId
        int id = 0;
        std::string code;
        std::string::size_type idPos = stylesXml.find("numFmtId=\"", fmtPos);
        if (idPos != std::string::npos) {
            idPos += 10;
            std::string::size_type idEnd = stylesXml.find('"', idPos);
            if (idEnd != std::string::npos) {
                id = std::atoi(stylesXml.substr(idPos, idEnd - idPos).c_str());
            }
        }
        std::string::size_type codePos = stylesXml.find("formatCode=\"", fmtPos);
        if (codePos != std::string::npos) {
            codePos += 12;
            std::string::size_type codeEnd = stylesXml.find('"', codePos);
            if (codeEnd != std::string::npos) {
                code = stylesXml.substr(codePos, codeEnd - codePos);
            }
        }
        numFmts_.emplace_back(id, code);
        pos = fmtPos + 7;
    }
    return true;
}

bool XlsxConverter::parseWorksheet_(int sheetIndex) {
    if (sheetIndex < 0 || sheetIndex >= static_cast<int>(workbook_.sheets.size())) {
        return false;
    }

    // 通过 r:id 找到 worksheet 文件路径
    const auto& sheet = workbook_.sheets[sheetIndex];
    std::string wsPath;
    for (const auto& rel : workbook_.rels) {
        if (rel.first == sheet.relId) {
            wsPath = rel.second;
            break;
        }
    }
    if (wsPath.empty()) {
        // 回退：尝试默认路径 xl/worksheets/sheet{N}.xml
        wsPath = "xl/worksheets/sheet" + std::to_string(sheetIndex + 1) + ".xml";
    }

    if (!zip_->hasEntry(wsPath)) {
        err_ = "worksheet not found: " + wsPath;
        return false;
    }

    std::string wsXml = zip_->readEntryText(wsPath);
    if (wsXml.empty()) {
        err_ = "worksheet xml empty: " + wsPath;
        return false;
    }

    // SmlReader 复用，先清空之前的状态
    smlReader_ = std::make_unique<ooxml::sml::SmlReader>();
    if (!smlReader_->parseWorksheet(wsXml)) {
        err_ = "failed to parse worksheet: " + smlReader_->error();
        return false;
    }

    auto& result = sheetResults_[sheetIndex];
    result.dimension = smlReader_->dimension();
    result.cells = smlReader_->cells();
    result.merges = smlReader_->merges();

    return true;
}

json::JsonValue XlsxConverter::buildSheetJson_(int sheetIndex) {
    const auto& sheet = workbook_.sheets[sheetIndex];
    const auto& result = sheetResults_[sheetIndex];

    json::JsonValue obj = json::JsonValue::object();
    obj.set("sheetId", std::string("sheet_") + std::to_string(sheetIndex));
    obj.set("name", sheet.name);
    obj.set("index", static_cast<std::int64_t>(sheetIndex));

    // rowCount / columnCount：从 dimension 或 cells 推断
    std::uint32_t maxRow = 0, maxCol = 0;
    for (const auto& cell : result.cells) {
        if (cell.first.row > maxRow) maxRow = cell.first.row;
        if (cell.first.col > maxCol) maxCol = cell.first.col;
    }
    obj.set("rowCount", static_cast<std::int64_t>(maxRow));
    obj.set("columnCount", static_cast<std::int64_t>(maxCol));

    // 默认行高/列宽（Phase H4 暂用默认值）
    obj.set("defaultRowHeight", 20.0);
    obj.set("defaultColumnWidth", 64.0);

    // cells 数组
    json::JsonValue cellsArr = json::JsonValue::array();
    for (const auto& cell : result.cells) {
        cellsArr.append(buildCellJson_(cell.first, cell.second));
    }
    obj.set("cells", std::move(cellsArr));

    // mergedCells
    json::JsonValue mergesArr = json::JsonValue::array();
    for (const auto& merge : result.merges) {
        // 解析 "A1:C3" → startRow/startColumn/endRow/endColumn
        json::JsonValue m = json::JsonValue::object();
        // 简单拆分
        std::string ref = merge.ref;
        std::string::size_type colon = ref.find(':');
        if (colon != std::string::npos) {
            std::string start = ref.substr(0, colon);
            std::string end = ref.substr(colon + 1);
            auto startRef = ooxml::sml::SmlReader::parseCellRef(start);
            auto endRef = ooxml::sml::SmlReader::parseCellRef(end);
            m.set("startRow", static_cast<std::int64_t>(startRef.row - 1));  // 0-based
            m.set("startColumn", static_cast<std::int64_t>(startRef.col - 1));
            m.set("endRow", static_cast<std::int64_t>(endRef.row - 1));
            m.set("endColumn", static_cast<std::int64_t>(endRef.col - 1));
        }
        mergesArr.append(std::move(m));
    }
    obj.set("mergedCells", std::move(mergesArr));

    // columnWidths / rowHeights / images / charts：Phase H4 暂不处理
    obj.set("columnWidths", json::JsonValue::array());
    obj.set("rowHeights", json::JsonValue::array());
    obj.set("images", json::JsonValue::array());
    obj.set("charts", json::JsonValue::array());

    return obj;
}

json::JsonValue XlsxConverter::buildCellJson_(const ooxml::sml::CellRef& ref,
                                              const ooxml::sml::CellValue& value) {
    json::JsonValue cell = json::JsonValue::object();
    // 0-based 行列
    cell.set("row", static_cast<std::int64_t>(ref.row - 1));
    cell.set("column", static_cast<std::int64_t>(ref.col - 1));

    // 根据 CellType 处理值
    switch (value.type) {
        case ooxml::sml::CellType::kSharedString: {
            cell.set("type", std::string("string"));
            std::int64_t idx = value.sharedIndex;
            if (idx >= 0 && idx < static_cast<std::int64_t>(sharedStrings_.size())) {
                cell.set("value", sharedStrings_[idx]);
            } else {
                cell.set("value", std::string(""));
            }
            break;
        }
        case ooxml::sml::CellType::kInlineString: {
            cell.set("type", std::string("string"));
            cell.set("value", value.raw);
            break;
        }
        case ooxml::sml::CellType::kBoolean: {
            cell.set("type", std::string("boolean"));
            cell.set("value", value.raw == "1" || value.raw == "true");
            break;
        }
        case ooxml::sml::CellType::kError: {
            cell.set("type", std::string("error"));
            cell.set("value", value.raw);
            break;
        }
        case ooxml::sml::CellType::kNumber:
        case ooxml::sml::CellType::kDate:
        default: {
            // 尝试解析为数字
            if (!value.raw.empty()) {
                char* endPtr = nullptr;
                double d = std::strtod(value.raw.c_str(), &endPtr);
                if (endPtr != value.raw.c_str() && *endPtr == '\0') {
                    // 成功解析为数字
                    // 检查是否为整数
                    if (d == static_cast<double>(static_cast<std::int64_t>(d)) &&
                        std::abs(d) < 1e15) {
                        cell.set("type", std::string("number"));
                        cell.set("value", static_cast<std::int64_t>(d));
                    } else {
                        cell.set("type", std::string("number"));
                        cell.set("value", d);
                    }
                } else {
                    // 非数字，作为字符串
                    cell.set("type", std::string("string"));
                    cell.set("value", value.raw);
                }
            } else {
                cell.set("type", std::string("string"));
                cell.set("value", std::string(""));
            }
            break;
        }
    }

    // 样式（Phase H4 暂不处理，留空对象）
    cell.set("style", json::JsonValue::object());

    return cell;
}

json::JsonValue XlsxConverter::buildDefinedNamesJson_() {
    json::JsonValue arr = json::JsonValue::array();
    for (const auto& dn : workbook_.definedNames) {
        json::JsonValue item = json::JsonValue::object();
        item.set("name", dn.first);
        item.set("ref", dn.second);
        arr.append(std::move(item));
    }
    return arr;
}

json::JsonValue XlsxConverter::buildCorePropertiesJson_() {
    json::JsonValue obj = json::JsonValue::object();
    obj.set("title", coreProps_.title);
    obj.set("creator", coreProps_.creator);
    obj.set("created", coreProps_.created);
    obj.set("modified", coreProps_.modified);
    return obj;
}

bool XlsxConverter::isDateFormat_(int numFmtId, const std::string& formatCode) const {
    // 内置日期格式 ID：14-22, 27-36, 45-47, 50-58
    // 参考 [MS-OE376] 2.2.1
    if (numFmtId >= 14 && numFmtId <= 22) return true;
    if (numFmtId >= 27 && numFmtId <= 36) return true;
    if (numFmtId >= 45 && numFmtId <= 47) return true;
    if (numFmtId >= 50 && numFmtId <= 58) return true;
    // 自定义格式：委托 DateUtil
    if (!formatCode.empty()) {
        return numfmt::DateUtil::isADateFormat(numFmtId, formatCode);
    }
    return false;
}

std::string XlsxConverter::errorJson_(ErrorCode code, const std::string& message) {
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
