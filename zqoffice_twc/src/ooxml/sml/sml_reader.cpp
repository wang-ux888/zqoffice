// =============================================================================
// src/ooxml/sml/sml_reader.cpp
// =============================================================================
#include "sml_reader.h"

#include <cctype>
#include <cstdlib>

namespace zq {
namespace office {
namespace ooxml {
namespace sml {

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

bool SmlReader::parseWorkbook(const std::string& xml) {
    parsed_ = false;
    sheets_.clear();
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

    if (nodeLocalName_(root) != "workbook") {
        error_ = "expected workbook, got " + root.name();
        return false;
    }

    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        if (nodeLocalName_(child) == "sheets") {
            parseSheets_(child);
            break;
        }
    }

    parsed_ = true;
    return true;
}

bool SmlReader::parseWorksheet(const std::string& xml) {
    parsed_ = false;
    dimension_.clear();
    cells_.clear();
    merges_.clear();
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

    if (nodeLocalName_(root) != "worksheet") {
        error_ = "expected worksheet, got " + root.name();
        return false;
    }

    for (XmlNode child = root.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "dimension") {
            dimension_ = child.attribute("ref");
        } else if (lname == "sheetData") {
            parseSheetData_(child);
        } else if (lname == "mergeCells") {
            parseMergeCells_(child);
        }
    }

    parsed_ = true;
    return true;
}

bool SmlReader::parseSharedStrings(const std::string& xml) {
    parsed_ = false;
    sharedStrings_.clear();
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

    if (nodeLocalName_(root) != "sst") {
        error_ = "expected sst, got " + root.name();
        return false;
    }

    for (XmlNode si = root.firstChild(); si.valid();
         si = si.nextSibling()) {
        if (nodeLocalName_(si) == "si") {
            sharedStrings_.push_back(extractSharedString_(si));
        }
    }

    parsed_ = true;
    return true;
}

bool SmlReader::parseStyles(const std::string& xml) {
    parsed_ = false;
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

    if (nodeLocalName_(root) != "styleSheet") {
        error_ = "expected styleSheet, got " + root.name();
        return false;
    }

    // 详情由 Phase D 的 ThemeRuntimeBuilder/ChartConvert 处理
    // 这里仅标记解析成功，保留原始 XmlReader 供上层访问
    parsed_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// 内部解析
// ---------------------------------------------------------------------------

void SmlReader::parseSheets_(const XmlNode& sheetsNode) {
    for (XmlNode sheet = sheetsNode.firstChild(); sheet.valid();
         sheet = sheet.nextSibling()) {
        if (nodeLocalName_(sheet) != "sheet") continue;
        SheetItem item;
        item.name = sheet.attribute("name");
        item.sheetId = toUint32_(sheet.attribute("sheetId"));
        // r:id 属性（命名空间前缀可能是 r: 或无前缀）
        item.relId = sheet.attribute("r:id");
        if (item.relId.empty()) {
            item.relId = sheet.attribute("id");
        }
        sheets_.push_back(item);
    }
}

void SmlReader::parseSheetData_(const XmlNode& sheetData) {
    for (XmlNode row = sheetData.firstChild(); row.valid();
         row = row.nextSibling()) {
        if (nodeLocalName_(row) != "row") continue;
        std::uint32_t rowNum = toUint32_(row.attribute("r"));
        parseRow_(row, rowNum);
    }
}

void SmlReader::parseRow_(const XmlNode& row, std::uint32_t rowNum) {
    for (XmlNode cell = row.firstChild(); cell.valid();
         cell = cell.nextSibling()) {
        if (nodeLocalName_(cell) != "c") continue;
        // 从 r 属性解析引用（如 "A1"）
        CellRef ref = parseCellRef(cell.attribute("r"));
        if (!ref.valid) {
            // 没有 r 属性，根据行号和单元格序号推断列号
            // 简化：跳过这种边缘情况
            continue;
        }
        CellValue value = parseCell_(cell);
        cells_.emplace_back(ref, value);
    }
    (void)rowNum;  // rowNum 已通过 cell.r 提取，这里不再使用
}

CellValue SmlReader::parseCell_(const XmlNode& cell) {
    CellValue value;
    // t 属性
    std::string t = cell.attribute("t");
    if (t == "s") {
        value.type = CellType::kSharedString;
    } else if (t == "b") {
        value.type = CellType::kBoolean;
    } else if (t == "inlineStr" || t == "str") {
        value.type = CellType::kInlineString;
    } else if (t == "e") {
        value.type = CellType::kError;
    } else {
        value.type = CellType::kNumber;
    }

    // v 元素（数值或共享字符串索引）
    for (XmlNode child = cell.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "v") {
            value.raw = child.text();
            if (value.type == CellType::kSharedString) {
                value.sharedIndex = toInt64_(value.raw);
            }
        } else if (lname == "is") {
            // 内联字符串：<is><t>...</t></is>
            for (XmlNode tNode = child.firstChild(); tNode.valid();
                 tNode = tNode.nextSibling()) {
                if (nodeLocalName_(tNode) == "t") {
                    value.raw = tNode.text();
                    break;
                }
            }
        }
    }
    return value;
}

void SmlReader::parseMergeCells_(const XmlNode& mergeCells) {
    for (XmlNode mc = mergeCells.firstChild(); mc.valid();
         mc = mc.nextSibling()) {
        if (nodeLocalName_(mc) != "mergeCell") continue;
        MergeRange mr;
        mr.ref = mc.attribute("ref");
        merges_.push_back(mr);
    }
}

std::string SmlReader::extractSharedString_(const XmlNode& si) {
    // si 可能包含：
    //   <t>plain text</t>                 - 纯文本
    //   <r><rPr>..</rPr><t>run1</t></r>   - 富文本 runs
    // 富文本情况下合并所有 r/t 内容
    std::string result;
    for (XmlNode child = si.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string lname = nodeLocalName_(child);
        if (lname == "t") {
            result += child.text();
        } else if (lname == "r") {
            // run：查找子元素 t
            for (XmlNode rChild = child.firstChild(); rChild.valid();
                 rChild = rChild.nextSibling()) {
                if (nodeLocalName_(rChild) == "t") {
                    result += rChild.text();
                }
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// 静态工具
// ---------------------------------------------------------------------------

CellRef SmlReader::parseCellRef(const std::string& ref) {
    CellRef cr;
    if (ref.empty()) return cr;

    // 分离字母列和数字行
    std::size_t i = 0;
    while (i < ref.size() && (std::isalpha(static_cast<unsigned char>(ref[i])) ||
                              ref[i] == '$')) {
        ++i;
    }
    if (i == 0 || i == ref.size()) return cr;  // 无列字母或无行号

    std::string colPart = ref.substr(0, i);
    std::string rowPart = ref.substr(i);

    // 去除 $ 符号
    std::string colLetters;
    for (char c : colPart) {
        if (c != '$') colLetters.push_back(c);
    }

    cr.col = lettersToColumn(colLetters);
    try {
        cr.row = static_cast<std::uint32_t>(std::stoul(rowPart));
    } catch (...) {
        return cr;
    }
    cr.valid = (cr.col > 0 && cr.row > 0);
    return cr;
}

std::string SmlReader::columnToLetters(std::uint32_t col) {
    std::string result;
    while (col > 0) {
        --col;  // 1-based → 0-based
        result.insert(result.begin(),
                      static_cast<char>('A' + (col % 26)));
        col /= 26;
    }
    return result;
}

std::uint32_t SmlReader::lettersToColumn(const std::string& letters) {
    if (letters.empty()) return 0;
    std::uint32_t result = 0;
    for (char c : letters) {
        char uc = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (uc < 'A' || uc > 'Z') return 0;
        result = result * 26 + static_cast<std::uint32_t>(uc - 'A' + 1);
    }
    return result;
}

// ---------------------------------------------------------------------------
// 工具
// ---------------------------------------------------------------------------

std::string SmlReader::localName_(const std::string& qualified) {
    std::size_t pos = qualified.find(':');
    if (pos == std::string::npos) return qualified;
    return qualified.substr(pos + 1);
}

std::string SmlReader::nodeLocalName_(const XmlNode& node) {
    if (!node.valid()) return "";
    return localName_(node.name());
}

} // namespace sml
} // namespace ooxml
} // namespace office
} // namespace zq
