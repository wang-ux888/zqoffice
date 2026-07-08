// =============================================================================
// src/drawing/theme_runtime_builder.cpp
//
// ThemeRuntimeBuilder 实现
//
// 解析 OOXML 主题 XML（ppt/theme/theme1.xml）和二进制 OfficeArtTheme 记录
// =============================================================================
#include "theme_runtime_builder.h"

#include <cstring>

#include "ooxml/xml_reader.h"
#include "drawing/enums/prst_clr_type.h"

namespace zq {
namespace office {
namespace drawing {

namespace {

/// 读取小端 16 位无符号整数
inline std::uint16_t readU16LE(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}

/// 读取小端 32 位无符号整数
inline std::uint32_t readU32LE(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

/// 颜色方案子元素名称 → ThemeType 映射
struct ClrSchemeEntry {
    const char* localName;
    ThemeType type;
};

/// a:clrScheme 子元素 localName → ThemeType
/// dk1/lt1/dk2/lt2/accent1-6/hlink/folHlink
constexpr ClrSchemeEntry kClrSchemeEntries[] = {
    { "dk1",     ThemeType::kDark1 },
    { "lt1",     ThemeType::kLight1 },
    { "dk2",     ThemeType::kDark2 },
    { "lt2",     ThemeType::kLight2 },
    { "accent1", ThemeType::kAccent1 },
    { "accent2", ThemeType::kAccent2 },
    { "accent3", ThemeType::kAccent3 },
    { "accent4", ThemeType::kAccent4 },
    { "accent5", ThemeType::kAccent5 },
    { "accent6", ThemeType::kAccent6 },
    { "hlink",   ThemeType::kHyperlink },
    { "folHlink",ThemeType::kFollowedHyperlink },
};

/// 根据 localName 查找颜色方案 ThemeType
ThemeType lookupSchemeColor(const std::string& localName) {
    for (const auto& e : kClrSchemeEntries) {
        if (localName == e.localName) return e.type;
    }
    return ThemeType::kUnknown;
}

/// 大写转小写
char toLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    return c;
}

/// 字符串转小写
std::string toLower(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = toLowerChar(c);
    return r;
}

} // namespace

ThemeRuntimeBuilder::ThemeRuntimeBuilder() = default;
ThemeRuntimeBuilder::~ThemeRuntimeBuilder() = default;

// ---------------------------------------------------------------------------
// OOXML 解析入口
// ---------------------------------------------------------------------------

bool ThemeRuntimeBuilder::buildFromOOXML(const std::string& themeXml) {
    theme_.clear();
    error_.clear();
    success_ = false;

    if (themeXml.empty()) {
        error_ = "empty theme XML";
        return false;
    }

    ooxml::XmlReader reader;
    if (!reader.parse(themeXml)) {
        error_ = "XML parse error: " + reader.error();
        return false;
    }

    ooxml::XmlNode root = reader.root();
    if (!root.valid()) {
        error_ = "no root element";
        return false;
    }

    return parseThemeRoot_(root);
}

bool ThemeRuntimeBuilder::parseThemeRoot_(const ooxml::XmlNode& root) {
    // 检查根元素 localName 是否为 "theme"
    std::string rootLocal = localName_(root.name());
    if (rootLocal != "theme") {
        error_ = "root element is not 'theme': " + root.name();
        return false;
    }

    // 主题名称
    theme_.setName(root.attribute("name"));

    // 查找 a:themeElements 子元素
    ooxml::XmlNode themeElements = root.firstChild("themeElements");
    if (!themeElements.valid()) {
        // 尝试按 localName 查找（命名空间前缀可能不同）
        for (ooxml::XmlNode child = root.firstChild(); child.valid();
             child = child.nextSibling()) {
            if (localName_(child.name()) == "themeElements") {
                themeElements = child;
                break;
            }
        }
    }

    if (!themeElements.valid()) {
        error_ = "missing a:themeElements";
        return false;
    }

    // 遍历 themeElements 子元素：clrScheme / fontScheme / fmtScheme
    bool hasClrScheme = false;
    for (ooxml::XmlNode child = themeElements.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "clrScheme") {
            if (!parseColorScheme_(child)) {
                // 颜色方案解析失败不致命，继续
            } else {
                hasClrScheme = true;
            }
        } else if (ln == "fontScheme") {
            parseFontScheme_(child);
        } else if (ln == "fmtScheme") {
            parseFmtScheme_(child);
        }
    }

    if (!hasClrScheme) {
        error_ = "missing a:clrScheme";
        return false;
    }

    success_ = true;
    return true;
}

bool ThemeRuntimeBuilder::parseColorScheme_(const ooxml::XmlNode& clrSchemeNode) {
    // 遍历 a:clrScheme 子元素：dk1/lt1/dk2/lt2/accent1-6/hlink/folHlink
    for (ooxml::XmlNode child = clrSchemeNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());
        ThemeType type = lookupSchemeColor(ln);
        if (type == ThemeType::kUnknown) continue;

        std::string rgb = extractColorRgb_(child);
        if (!rgb.empty()) {
            theme_.setColor(type, rgb);
        }
    }

    return theme_.isValid();
}

bool ThemeRuntimeBuilder::parseFontScheme_(const ooxml::XmlNode& fontSchemeNode) {
    FontScheme scheme;

    // 遍历 a:fontScheme 子元素：majorFont / minorFont
    for (ooxml::XmlNode child = fontSchemeNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "majorFont") {
            // 遍历 majorFont 子元素：latin / ea / cs
            for (ooxml::XmlNode f = child.firstChild(); f.valid();
                 f = f.nextSibling()) {
                std::string fln = localName_(f.name());
                if (fln == "latin") {
                    scheme.majorLatin = f.attribute("typeface");
                } else if (fln == "ea") {
                    scheme.majorEastAsian = f.attribute("typeface");
                } else if (fln == "cs") {
                    scheme.majorComplexScript = f.attribute("typeface");
                }
            }
        } else if (ln == "minorFont") {
            for (ooxml::XmlNode f = child.firstChild(); f.valid();
                 f = f.nextSibling()) {
                std::string fln = localName_(f.name());
                if (fln == "latin") {
                    scheme.minorLatin = f.attribute("typeface");
                } else if (fln == "ea") {
                    scheme.minorEastAsian = f.attribute("typeface");
                } else if (fln == "cs") {
                    scheme.minorComplexScript = f.attribute("typeface");
                }
            }
        }
    }

    scheme.valid = !scheme.majorLatin.empty() || !scheme.minorLatin.empty();
    theme_.setFontScheme(scheme);
    return scheme.valid;
}

bool ThemeRuntimeBuilder::parseFmtScheme_(const ooxml::XmlNode& fmtSchemeNode) {
    FmtScheme scheme;

    // 遍历 a:fmtScheme 子元素：fillStyleLst / lnStyleLst / effectStyleLst / bgFillStyleLst
    for (ooxml::XmlNode child = fmtSchemeNode.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "fillStyleLst") {
            int idx = 0;
            for (ooxml::XmlNode s = child.firstChild(); s.valid();
                 s = s.nextSibling()) {
                scheme.fillStyleIds.push_back(idx++);
            }
        } else if (ln == "lnStyleLst") {
            int idx = 0;
            for (ooxml::XmlNode s = child.firstChild(); s.valid();
                 s = s.nextSibling()) {
                scheme.lineStyleIds.push_back(idx++);
            }
        } else if (ln == "effectStyleLst") {
            int idx = 0;
            for (ooxml::XmlNode s = child.firstChild(); s.valid();
                 s = s.nextSibling()) {
                scheme.effectStyleIds.push_back(idx++);
            }
        } else if (ln == "bgFillStyleLst") {
            int idx = 0;
            for (ooxml::XmlNode s = child.firstChild(); s.valid();
                 s = s.nextSibling()) {
                scheme.bgFillStyleIds.push_back(idx++);
            }
        }
    }

    scheme.valid = !scheme.fillStyleIds.empty();
    theme_.setFmtScheme(scheme);
    return scheme.valid;
}

std::string ThemeRuntimeBuilder::extractColorRgb_(const ooxml::XmlNode& colorParent) {
    // 颜色节点结构：<colorSlot><a:srgbClr val="4472C4"/></colorSlot>
    // 或：<colorSlot><a:sysClr val="windowText" lastClr="000000"/></colorSlot>
    // 或：<colorSlot><a:prstClr val="aliceBlue"/></colorSlot>
    // 或：<colorSlot><a:schemeClr val="accent1"/></colorSlot>
    for (ooxml::XmlNode child = colorParent.firstChild(); child.valid();
         child = child.nextSibling()) {
        std::string ln = localName_(child.name());

        if (ln == "srgbClr") {
            // srgbClr val="RRGGBB"
            std::string val = child.attribute("val");
            if (!val.empty()) {
                return toLower(val);
            }
        } else if (ln == "sysClr") {
            // sysClr val="windowText" lastClr="000000"
            std::string lastClr = child.attribute("lastClr");
            if (!lastClr.empty()) {
                return toLower(lastClr);
            }
            // 没有 lastClr，使用 sysClr val 查找系统颜色 RGB
            // （简化处理：sysClr 通常会有 lastClr 属性）
        } else if (ln == "prstClr") {
            // prstClr val="aliceBlue"
            std::string val = child.attribute("val");
            PrstClrType pt = prstClrTypeFromString(val);
            if (pt != PrstClrType::kUnknown) {
                const char* rgb = prstClrTypeToRgb(pt);
                if (rgb[0] != '\0') {
                    return toLower(std::string(rgb));
                }
            }
        } else if (ln == "schemeClr") {
            // schemeClr val="accent1" - 递归查找主题中的颜色
            // 简化处理：先不实现递归，依赖颜色方案已加载
            // 实际场景中 clrScheme 子元素不会用 schemeClr
        }
    }
    return "";
}

std::string ThemeRuntimeBuilder::localName_(const std::string& fullName) {
    // 去掉命名空间前缀：a:dk1 → dk1
    auto pos = fullName.find(':');
    if (pos == std::string::npos) return fullName;
    return fullName.substr(pos + 1);
}

// ---------------------------------------------------------------------------
// 二进制解析入口
// ---------------------------------------------------------------------------

bool ThemeRuntimeBuilder::buildFromBinary(const unsigned char* data, std::size_t size) {
    theme_.clear();
    error_.clear();
    success_ = false;

    if (!data || size == 0) {
        error_ = "empty binary data";
        return false;
    }

    return parseBinaryTheme_(data, size);
}

bool ThemeRuntimeBuilder::parseBinaryTheme_(const unsigned char* data, std::size_t size) {
    // 旧版 .ppt/.xls 的主题存储在 OfficeArtTheme 记录中
    // [MS-ODRAW] section 2.5.4 OfficeArtColorSchemeHeaderStructure
    //
    // 简化实现：仅解析颜色方案部分
    // 完整的 OfficeArtTheme 解析需要 Escher 容器递归解析
    //
    // 二进制结构：
    //   - OfficeArtRecordHeader (8 bytes): ver=0, instance=0, type=0xF000 (DggContainer)
    //   - 后续为子记录，其中包含 ColorSchemeContainer

    if (size < 8) {
        error_ = "binary data too small for OfficeArtRecordHeader";
        return false;
    }

    // 解析 Escher 记录头
    // verInstance (2 bytes) + recType (2 bytes) + recLen (4 bytes)
    std::uint16_t verInstance = readU16LE(data);
    std::uint16_t recType = readU16LE(data + 2);
    std::uint32_t recLen = readU32LE(data + 4);
    (void)verInstance;  // 暂不使用

    // 检查是否为 DggContainer (0xF000)
    if (recType != 0xF000) {
        error_ = "not a DggContainer (expected 0xF000)";
        return false;
    }

    if (8 + recLen > size) {
        error_ = "record length exceeds data size";
        return false;
    }

    // 遍历子记录查找 ColorScheme
    // 简化实现：直接在子记录中查找颜色数据
    std::size_t offset = 8;
    std::size_t end = 8 + recLen;

    while (offset + 8 <= end) {
        std::uint16_t subVerInstance = readU16LE(data + offset);
        std::uint16_t subRecType = readU16LE(data + offset + 2);
        std::uint32_t subRecLen = readU32LE(data + offset + 4);
        (void)subVerInstance;  // 暂不使用

        if (offset + 8 + subRecLen > end) break;

        // ColorSchemeContainer 通常 type=0xF008 或包含 4 字节颜色数据
        // 这里简化：假设 ColorScheme 数据为 12 个 RGB 值（4 bytes each: RGBA 或 RGB0）
        if (subRecType == 0xF018 && subRecLen >= 12 * 4) {
            // 尝试解析颜色方案
            parseBinaryColorScheme_(data + offset + 8, subRecLen);
        }

        offset += 8 + subRecLen;
    }

    // 二进制解析即使没找到颜色方案，也返回 true（部分成功）
    // 完整实现需要递归解析 OfficeArtThemeContainer
    success_ = theme_.isValid();
    if (!success_) {
        // 二进制主题解析失败不致命，返回 true 但标记未成功
        // 上层可以根据 isSuccess() 判断
        return true;
    }
    return true;
}

bool ThemeRuntimeBuilder::parseBinaryColorScheme_(const unsigned char* data, std::size_t size) {
    // 简化实现：假设数据为连续的 RGB 值（每个 4 字节，BGR0 或 RGB0 格式）
    // 顺序：dk1, lt1, dk2, lt2, accent1, accent2, accent3, accent4, accent5, accent6, hlink, folHlink
    if (size < 12 * 4) return false;

    const ThemeType order[] = {
        ThemeType::kDark1, ThemeType::kLight1,
        ThemeType::kDark2, ThemeType::kLight2,
        ThemeType::kAccent1, ThemeType::kAccent2,
        ThemeType::kAccent3, ThemeType::kAccent4,
        ThemeType::kAccent5, ThemeType::kAccent6,
        ThemeType::kHyperlink, ThemeType::kFollowedHyperlink,
    };

    char buf[7];
    for (int i = 0; i < 12; ++i) {
        std::uint32_t bgr0 = readU32LE(data + i * 4);
        // BGR0 格式：高字节为 0，低 3 字节为 BGR
        std::uint8_t b = (bgr0 >> 0) & 0xFF;
        std::uint8_t g = (bgr0 >> 8) & 0xFF;
        std::uint8_t r = (bgr0 >> 16) & 0xFF;
        std::snprintf(buf, sizeof(buf), "%02x%02x%02x", r, g, b);
        theme_.setColor(order[i], buf);
    }
    return true;
}

} // namespace drawing
} // namespace office
} // namespace zq
