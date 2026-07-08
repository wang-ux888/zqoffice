// =============================================================================
//
// ZQOfficeExport / ZQOfficeImport / ZQExportFile C API 绑定
// （Phase H-Part6 / H10）
//
// 三个格式无关的通用入口，内部根据文件扩展名或 formatName 参数分派到
// 具体转换器（XlsxConverter/PptxConverter/DocxConverter）：
//
//   ZQOfficeExport(sourcePath, outputPath, optionsJson)
//     → 读取 sourcePath（Office 文件）→ ZQ JSON → 写入 outputPath 文件
//
//   ZQOfficeImport(clientVarsJson, outputPath, optionsJson)
//     → 读取 clientVarsJson（ZQ JSON 字符串）→ 写入 outputPath（Office 文件）
//
//   ZQExportFile(sourcePath, outputPath, formatName, optionsJson)
//     → 读取 sourcePath（Office 文件）→ ZQ JSON → 写入 outputPath（指定格式 Office 文件）
// =============================================================================
#include "zq/office/office.h"
#include "zq/office/types.h"
#include "zq/office/version.h"
#include "string_buffer.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "api/converters/docx_converter.h"
#include "api/converters/pptx_converter.h"
#include "api/converters/xlsx_converter.h"
#include "api/json/json.h"
#include "utils/format_selector.h"

namespace zq {
namespace office {
namespace api {

/// 构造成功 ExportResult JSON 字符串
static std::string successJson(const std::string& message = "ok",
                                  const std::string& data = "") {
    json::JsonValue result = json::JsonValue::object();
    result.set("code", static_cast<std::int64_t>(ErrorCode::OK));
    result.set("message", message);
    result.set("data", data);
    result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
    return json::serialize(result);
}

/// 构造错误 ExportResult JSON 字符串
static std::string errorJson(ErrorCode code, const std::string& message) {
    json::JsonValue result = json::JsonValue::object();
    result.set("code", static_cast<std::int64_t>(code));
    result.set("message", message);
    result.set("data", std::string(""));
    result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
    return json::serialize(result);
}

/// 从文件读取全部内容到字符串
static bool readFileContent(const std::string& path, std::string* out) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return false;
    std::ostringstream oss;
    oss << ifs.rdbuf();
    *out = oss.str();
    return true;
}

/// 将字符串写入文件
static bool writeFileContent(const std::string& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    return ofs.good();
}

/// 提取文件扩展名（小写，含点号，如 ".xlsx"）
static std::string getExtension(const std::string& path) {
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = path.substr(pos);
    // 转小写
    for (auto& c : ext) {
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    }
    return ext;
}

/// 根据扩展名获取目标格式（用于 fromJson 分派）
/// @return FileFormat 枚举值，Unknown 表示无法识别
static FileFormat formatFromExt(const std::string& ext) {
    if (ext == ".xlsx") return FileFormat::Xlsx;
    if (ext == ".pptx") return FileFormat::Pptx;
    if (ext == ".docx") return FileFormat::Docx;
    return FileFormat::Unknown;
}

/// 根据格式名称获取目标格式（用于 ZQExportFile 的 formatName 参数）
/// @param formatName  "xlsx" / "pptx" / "docx"（大小写不敏感）
static FileFormat formatFromName(const std::string& formatName) {
    std::string lower = formatName;
    for (auto& c : lower) {
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    }
    if (lower == "xlsx") return FileFormat::Xlsx;
    if (lower == "pptx") return FileFormat::Pptx;
    if (lower == "docx") return FileFormat::Docx;
    return FileFormat::Unknown;
}

/// 调用对应转换器的 toJson（读取 Office 文件 → ZQ JSON）
static std::string dispatchToJson(FileFormat ff,
                                     const std::string& filePath,
                                     const std::string& dataDir,
                                     const std::string& optionsJson) {
    switch (ff) {
        case FileFormat::Xlsx: {
            converters::XlsxConverter conv;
            return conv.toJson(filePath, dataDir, optionsJson);
        }
        case FileFormat::Pptx: {
            converters::PptxConverter conv;
            return conv.toJson(filePath, dataDir, optionsJson);
        }
        case FileFormat::Docx: {
            converters::DocxConverter conv;
            return conv.toJson(filePath, dataDir, optionsJson);
        }
        default:
            return errorJson(ErrorCode::UnsupportedFormat,
                             "unsupported source format");
    }
}

/// 调用对应转换器的 fromJson（ZQ JSON → Office 文件）
static std::string dispatchFromJson(FileFormat ff,
                                       const std::string& clientVarsJson,
                                       const std::string& outputPath,
                                       const std::string& dataDir,
                                       const std::string& optionsJson) {
    switch (ff) {
        case FileFormat::Xlsx: {
            converters::XlsxConverter conv;
            return conv.fromJson(clientVarsJson, outputPath, dataDir, optionsJson);
        }
        case FileFormat::Pptx: {
            converters::PptxConverter conv;
            return conv.fromJson(clientVarsJson, outputPath, dataDir, optionsJson);
        }
        case FileFormat::Docx: {
            converters::DocxConverter conv;
            return conv.fromJson(clientVarsJson, outputPath, dataDir, optionsJson);
        }
        default:
            return errorJson(ErrorCode::UnsupportedFormat,
                             "unsupported target format");
    }
}

} // namespace api
} // namespace office
} // namespace zq

extern "C" {

// =============================================================================
// ZQOfficeExport：Office 文件 → ZQ JSON 文件
// =============================================================================

ZQ_OFFICE_API const char* ZQ_OFFICE_CALL
ZQOfficeExport(const char* sourcePath,
                    const char* outputPath,
                    const char* optionsJson) {
    using namespace zq::office;
    using namespace zq::office::api;

    if (!sourcePath || !*sourcePath) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::InvalidArgument, "sourcePath is null or empty"));
    }
    if (!outputPath || !*outputPath) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::InvalidArgument, "outputPath is null or empty"));
    }

    std::string src = sourcePath;
    std::string dst = outputPath;
    std::string opts = optionsJson ? optionsJson : "{}";

    // 1. 检测源文件格式
    FileFormat ff = FormatSelector::detectByFilePath(src);
    if (ff == FileFormat::Unknown) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::UnsupportedFormat,
                      "unsupported source format: " + getExtension(src)));
    }

    // 2. 调用对应转换器读取 Office 文件 → ZQ JSON 字符串
    std::string zqJson = dispatchToJson(ff, src, "", opts);
    // zqJson 本身是 ExportResult 包装（{code,message,data,version}）

    // 2.1 检查转换是否成功（code:0），失败则直接返回错误，不写入文件
    auto parsed = zq::office::json::parse(zqJson);
    if (parsed.isObject()) {
        auto* codeField = parsed.find("code");
        if (codeField && codeField->asInt() != 0) {
            return StringBufferAllocator::instance().allocate(zqJson);
        }
    }

    // 3. 将 ZQ JSON 写入 outputPath 文件
    if (!writeFileContent(dst, zqJson)) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::InternalError,
                      "failed to write output file: " + dst));
    }

    // 4. 返回成功 ExportResult
    return StringBufferAllocator::instance().allocate(successJson("ok", zqJson));
}

// =============================================================================
// ZQOfficeImport：ZQ JSON 字符串 → Office 文件
// =============================================================================

ZQ_OFFICE_API const char* ZQ_OFFICE_CALL
ZQOfficeImport(const char* clientVarsJson,
                    const char* outputPath,
                    const char* optionsJson) {
    using namespace zq::office;
    using namespace zq::office::api;

    if (!clientVarsJson || !*clientVarsJson) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::InvalidArgument, "clientVarsJson is null or empty"));
    }
    if (!outputPath || !*outputPath) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::InvalidArgument, "outputPath is null or empty"));
    }

    std::string json = clientVarsJson;
    std::string dst = outputPath;
    std::string opts = optionsJson ? optionsJson : "{}";

    // 1. 检测目标文件格式（根据 outputPath 扩展名）
    FileFormat ff = formatFromExt(getExtension(dst));
    if (ff == FileFormat::Unknown) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::UnsupportedFormat,
                      "unsupported output format: " + getExtension(dst)));
    }

    // 2. 调用对应转换器 fromJson（ZQ JSON → Office 文件）
    std::string result = dispatchFromJson(ff, json, dst, "", opts);

    // 3. 返回转换器结果（已是 ExportResult JSON）
    return StringBufferAllocator::instance().allocate(result);
}

// =============================================================================
// ZQExportFile：Office 文件 → 指定格式 Office 文件（格式转换）
// =============================================================================

ZQ_OFFICE_API const char* ZQ_OFFICE_CALL
ZQExportFile(const char* sourcePath,
               const char* outputPath,
               const char* formatName,
               const char* optionsJson) {
    using namespace zq::office;
    using namespace zq::office::api;

    if (!sourcePath || !*sourcePath) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::InvalidArgument, "sourcePath is null or empty"));
    }
    if (!outputPath || !*outputPath) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::InvalidArgument, "outputPath is null or empty"));
    }

    std::string src = sourcePath;
    std::string dst = outputPath;
    std::string opts = optionsJson ? optionsJson : "{}";

    // 1. 检测源文件格式
    FileFormat srcFmt = FormatSelector::detectByFilePath(src);
    if (srcFmt == FileFormat::Unknown) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::UnsupportedFormat,
                      "unsupported source format: " + getExtension(src)));
    }

    // 2. 确定目标格式：优先使用 formatName，否则根据 outputPath 扩展名
    FileFormat dstFmt = FileFormat::Unknown;
    if (formatName && *formatName) {
        dstFmt = formatFromName(formatName);
    }
    if (dstFmt == FileFormat::Unknown) {
        dstFmt = formatFromExt(getExtension(dst));
    }
    if (dstFmt == FileFormat::Unknown) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::UnsupportedFormat,
                      "cannot determine target format"));
    }

    // 3. 同格式直接复制（无需转换）
    if (srcFmt == dstFmt) {
        std::string content;
        if (!readFileContent(src, &content)) {
            return StringBufferAllocator::instance().allocate(
                errorJson(ErrorCode::FileNotFound,
                          "failed to read source: " + src));
        }
        if (!writeFileContent(dst, content)) {
            return StringBufferAllocator::instance().allocate(
                errorJson(ErrorCode::InternalError,
                          "failed to write output: " + dst));
        }
        return StringBufferAllocator::instance().allocate(successJson("ok", ""));
    }

    // 4. 读取源文件 → ZQ JSON
    std::string zqResult = dispatchToJson(srcFmt, src, "", opts);
    // zqResult 是 ExportResult JSON，需提取 data 字段
    auto parsed = zq::office::json::parse(zqResult);
    if (!parsed.isObject()) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::ConversionError, "failed to parse conversion result"));
    }
    auto* codeField = parsed.find("code");
    if (!codeField || codeField->asInt() != 0) {
        // 读取失败，返回原始错误
        return StringBufferAllocator::instance().allocate(zqResult);
    }
    auto* dataField = parsed.find("data");
    if (!dataField || !dataField->isString()) {
        return StringBufferAllocator::instance().allocate(
            errorJson(ErrorCode::ConversionError, "missing data field in conversion result"));
    }
    std::string zqJson = dataField->asString();

    // 5. ZQ JSON → 目标格式文件
    std::string result = dispatchFromJson(dstFmt, zqJson, dst, "", opts);
    return StringBufferAllocator::instance().allocate(result);
}

} // extern "C"
