// =============================================================================
// src/utils/format_selector.cpp
// =============================================================================
#include "format_selector.h"
#include "zq/office/constants.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>

namespace zq {
namespace office {

// ---------------------------------------------------------------------------
// 静态集合定义
// ---------------------------------------------------------------------------
const std::set<std::string> FormatSelector::XLS_EXT  = {".xlsx", ".xls", ".csv"};
const std::set<std::string> FormatSelector::PPT_EXT  = {".pptx", ".ppt"};
const std::set<std::string> FormatSelector::WORD_EXT = {".docx", ".doc"};
const std::set<std::string> FormatSelector::TXT_EXT  = {".txt"};

// ---------------------------------------------------------------------------
// 工具：将字符串转小写
// ---------------------------------------------------------------------------
static std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

// ---------------------------------------------------------------------------
// 工具：从文件路径提取扩展名（小写，含点）
// ---------------------------------------------------------------------------
static std::string extractExt(const std::string& path) {
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) return std::string();
    return toLower(path.substr(pos));
}

// ---------------------------------------------------------------------------
// 魔数检测
// ---------------------------------------------------------------------------
bool FormatSelector::isCompressedFile(std::int64_t magic) {
    // ZIP magic：前 4 字节 0x50 0x4B 0x03 0x04（小端存储）
    // 即低 32 位 == 0x04034B50
    return (static_cast<std::uint64_t>(magic) & 0xFFFFFFFFu) == constants::kZipMagic;
}

bool FormatSelector::isOle2File(std::int64_t magic) {
    // OLE2 magic：D0 CF 11 E0 A1 B1 1A E1（小端存储）
    return static_cast<std::uint64_t>(magic) == constants::kOle2Magic;
}

bool FormatSelector::isExcelExt(const std::string& ext) {
    return XLS_EXT.find(toLower(ext)) != XLS_EXT.end();
}

bool FormatSelector::isPptExt(const std::string& ext) {
    return PPT_EXT.find(toLower(ext)) != PPT_EXT.end();
}

bool FormatSelector::isWordExt(const std::string& ext) {
    return WORD_EXT.find(toLower(ext)) != WORD_EXT.end();
}

bool FormatSelector::isTxtExt(const std::string& ext) {
    return TXT_EXT.find(toLower(ext)) != TXT_EXT.end();
}

FileFormat FormatSelector::detectByFilePath(const std::string& filePath) {
    std::string ext = extractExt(filePath);

    // 先按扩展名分类大类
    bool isExcel = isExcelExt(ext);
    bool isPpt   = isPptExt(ext);
    bool isWord  = isWordExt(ext);

    if (!isExcel && !isPpt && !isWord) {
        if (isTxtExt(ext)) return FileFormat::Csv; // .txt 与 .csv 一起处理
        return FileFormat::Unknown;
    }

    // 读取文件头
    std::ifstream f(filePath, std::ios::binary);
    if (!f.is_open()) {
        // 文件打不开时仅按扩展名判断
        if (isExcel) {
            if (ext == ".csv") return FileFormat::Csv;
            if (ext == ".xls") return FileFormat::Xls;
            return FileFormat::Xlsx;
        }
        if (isPpt) return (ext == ".ppt") ? FileFormat::Ppt : FileFormat::Pptx;
        if (isWord) return (ext == ".doc") ? FileFormat::Doc : FileFormat::Docx;
        return FileFormat::Unknown;
    }

    std::vector<unsigned char> header(8, 0);
    f.read(reinterpret_cast<char*>(header.data()), 8);
    std::int64_t magic = 0;
    for (int i = 7; i >= 0; --i) {
        magic = (magic << 8) | header[i];
    }

    bool isZip  = isCompressedFile(magic);
    bool isOle2 = isOle2File(magic);

    if (isExcel) {
        if (ext == ".csv") return FileFormat::Csv;
        if (ext == ".xls")  return isOle2 ? FileFormat::Xls  : FileFormat::Unknown;
        if (ext == ".xlsx") return isZip  ? FileFormat::Xlsx : FileFormat::Unknown;
        return FileFormat::Unknown;
    }
    if (isPpt) {
        if (ext == ".ppt")  return isOle2 ? FileFormat::Ppt  : FileFormat::Unknown;
        if (ext == ".pptx") return isZip  ? FileFormat::Pptx : FileFormat::Unknown;
        return FileFormat::Unknown;
    }
    if (isWord) {
        if (ext == ".doc")  return isOle2 ? FileFormat::Doc  : FileFormat::Unknown;
        if (ext == ".docx") return isZip  ? FileFormat::Docx : FileFormat::Unknown;
        return FileFormat::Unknown;
    }
    return FileFormat::Unknown;
}

FileFormat FormatSelector::detectByMagic(std::int64_t magic) {
    if (isCompressedFile(magic)) {
        // ZIP 容器：xlsx/pptx/docx 三者头 8 字节相同，需打开 ZIP 内 [Content_Types].xml 才能区分
        // 此处仅返回 Xlsx 作为占位（实际由上层 api/detect.cpp 完成 ZIP 内部检测）
        return FileFormat::Xlsx;
    }
    if (isOle2File(magic)) {
        // OLE2 容器：xls/ppt/doc 三者头相同，需读取 OLE2 流才能区分
        return FileFormat::Xls;
    }
    return FileFormat::Unknown;
}

} // namespace office
} // namespace zq
