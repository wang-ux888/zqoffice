// =============================================================================
// src/ooxml/opc/opc_package.cpp
// =============================================================================
#include "opc_package.h"

#include "../xml_reader.h"

#include <algorithm>

namespace zq {
namespace office {
namespace ooxml {
namespace opc {

// ---------------------------------------------------------------------------
// Relationship
// ---------------------------------------------------------------------------

std::string Relationship::typeShortName() const {
    if (type.empty()) return "";
    // 取最后一个 '/' 之后的部分
    std::size_t pos = type.rfind('/');
    if (pos == std::string::npos) return type;
    return type.substr(pos + 1);
}

// ---------------------------------------------------------------------------
// 打开 / 关闭
// ---------------------------------------------------------------------------

bool OpcPackage::openFromMemory(const std::vector<unsigned char>& data) {
    return zip_.openFromMemory(data);
}

bool OpcPackage::openFromMemory(const unsigned char* data, std::size_t size) {
    return zip_.openFromMemory(data, size);
}

bool OpcPackage::openFromFile(const std::string& filePath) {
    return zip_.open(filePath);
}

void OpcPackage::close() {
    zip_.close();
}

bool OpcPackage::isOpen() const {
    return zip_.isOpen();
}

// ---------------------------------------------------------------------------
// Part 访问
// ---------------------------------------------------------------------------

std::vector<unsigned char> OpcPackage::getPart(
    const std::string& partPath) const {
    std::string normalized = normalizePath_(partPath);
    return zip_.readEntry(normalized);
}

std::string OpcPackage::getPartText(const std::string& partPath) const {
    std::string normalized = normalizePath_(partPath);
    return zip_.readEntryText(normalized);
}

bool OpcPackage::hasPart(const std::string& partPath) const {
    std::string normalized = normalizePath_(partPath);
    return zip_.hasEntry(normalized);
}

std::vector<std::string> OpcPackage::parts() const {
    std::vector<std::string> result;
    if (!zip_.isOpen()) return result;
    const auto& entries = zip_.entries();
    result.reserve(entries.size());
    for (const auto& e : entries) {
        if (e.isFile()) {
            result.push_back(e.name);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// 关系访问
// ---------------------------------------------------------------------------

std::vector<Relationship> OpcPackage::getRootRelationships() const {
    return getRelationships("");  // 顶层关系文件为 _rels/.rels
}

std::vector<Relationship> OpcPackage::getRelationships(
    const std::string& partPath) const {
    std::string relsPath = relsPathForPart_(partPath);
    if (!zip_.hasEntry(relsPath)) {
        return {};
    }
    std::string relsXml = zip_.readEntryText(relsPath);
    return parseRelationships_(relsXml);
}

std::string OpcPackage::resolveByType(const std::string& relTypeShort) const {
    auto rels = getRootRelationships();
    for (const auto& r : rels) {
        if (r.typeShortName() == relTypeShort) {
            return normalizePath_(r.target);
        }
    }
    return "";
}

std::string OpcPackage::resolveById(const std::string& relId) const {
    auto rels = getRootRelationships();
    for (const auto& r : rels) {
        if (r.id == relId) {
            return normalizePath_(r.target);
        }
    }
    return "";
}

// ---------------------------------------------------------------------------
// 内部工具
// ---------------------------------------------------------------------------

std::vector<Relationship> OpcPackage::parseRelationships_(
    const std::string& relsXml) const {
    std::vector<Relationship> result;
    if (relsXml.empty()) return result;

    XmlReader reader;
    if (!reader.parse(relsXml)) {
        return result;
    }

    XmlNode root = reader.root();
    if (!root.valid()) return result;

    // 遍历 Relationship 子节点
    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname;
        std::string name = child.name();
        std::size_t pos = name.rfind(':');
        if (pos != std::string::npos) {
            lname = name.substr(pos + 1);
        } else {
            lname = name;
        }

        if (lname != "Relationship") continue;

        Relationship rel;
        rel.id = child.attribute("Id");
        rel.type = child.attribute("Type");
        rel.target = child.attribute("Target");
        rel.targetMode = child.attribute("TargetMode");
        result.push_back(rel);
    }
    return result;
}

std::string OpcPackage::relsPathForPart_(const std::string& partPath) const {
    if (partPath.empty()) {
        // 顶层关系
        return "_rels/.rels";
    }

    std::string normalized = normalizePath_(partPath);

    // 在 partPath 的目录下创建 _rels/<filename>.rels
    // 例如 "xl/workbook.xml" → "xl/_rels/workbook.xml.rels"
    std::size_t slashPos = normalized.rfind('/');
    std::string dir;
    std::string filename;
    if (slashPos == std::string::npos) {
        dir = "";
        filename = normalized;
    } else {
        dir = normalized.substr(0, slashPos);
        filename = normalized.substr(slashPos + 1);
    }

    if (dir.empty()) {
        return "_rels/" + filename + ".rels";
    } else {
        return dir + "/_rels/" + filename + ".rels";
    }
}

std::string OpcPackage::normalizePath_(const std::string& path) {
    std::string result = path;
    // 去除前导 /
    while (!result.empty() && result[0] == '/') {
        result.erase(0, 1);
    }
    // 去除 ./ 前缀
    while (result.size() >= 2 && result[0] == '.' && result[1] == '/') {
        result.erase(0, 2);
    }
    return result;
}

std::string OpcPackage::resolveTarget_(const std::string& base,
                                         const std::string& target) {
    // 如果 target 以 / 开头，视为绝对路径
    if (!target.empty() && target[0] == '/') {
        return normalizePath_(target);
    }
    // 否则相对 base 所在目录
    std::string baseDir;
    std::size_t slashPos = base.rfind('/');
    if (slashPos != std::string::npos) {
        baseDir = base.substr(0, slashPos);
    }
    if (baseDir.empty()) {
        return normalizePath_(target);
    }
    return normalizePath_(baseDir + "/" + target);
}

} // namespace opc
} // namespace ooxml
} // namespace office
} // namespace zq
