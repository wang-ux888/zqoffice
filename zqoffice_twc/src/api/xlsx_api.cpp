// =============================================================================
// src/api/xlsx_api.cpp
//
// XlsxToZQSheet C API 绑定（Phase H4）
//
// 将 C API 调用委托给 XlsxConverter
// =============================================================================
#include "zq/office/office.h"
#include "zq/office/types.h"
#include "zq/office/version.h"
#include "string_buffer.h"

#include <string>

#include "api/converters/xlsx_converter.h"
#include "api/json/json.h"

extern "C" {

ZQ_OFFICE_API const char* ZQ_OFFICE_CALL
XlsxToZQSheet(const char* filePath,
                const char* dataDirectory,
                const char* reserved,
                const char* optionsJson) {
    using namespace zq::office;

    if (!filePath || !*filePath) {
        // 构造错误 JSON
        json::JsonValue result = json::JsonValue::object();
        result.set("code", static_cast<std::int64_t>(ErrorCode::InvalidArgument));
        result.set("message", std::string("filePath is null or empty"));
        result.set("data", std::string(""));
        result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
        std::string s = json::serialize(result);
        return api::StringBufferAllocator::instance().allocate(s);
    }

    (void)reserved;  // 预留参数

    converters::XlsxConverter conv;
    std::string resultJson = conv.toJson(
        filePath,
        dataDirectory ? dataDirectory : "",
        optionsJson ? optionsJson : "{}");

    return api::StringBufferAllocator::instance().allocate(resultJson);
}

// =============================================================================
// ZQSheetToXlsx（Phase H5）：ZQ Sheet JSON → xlsx 文件
// =============================================================================

ZQ_OFFICE_API const char* ZQ_OFFICE_CALL
ZQSheetToXlsx(const char* clientVarsJson,
                const char* outputPath,
                const char* dataDirectory,
                const char* reserved,
                const char* optionsJson) {
    using namespace zq::office;

    (void)reserved;  // 预留参数

    if (!clientVarsJson || !*clientVarsJson) {
        json::JsonValue result = json::JsonValue::object();
        result.set("code", static_cast<std::int64_t>(ErrorCode::InvalidArgument));
        result.set("message", std::string("clientVarsJson is null or empty"));
        result.set("data", std::string(""));
        result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
        std::string s = json::serialize(result);
        return api::StringBufferAllocator::instance().allocate(s);
    }

    if (!outputPath || !*outputPath) {
        json::JsonValue result = json::JsonValue::object();
        result.set("code", static_cast<std::int64_t>(ErrorCode::InvalidArgument));
        result.set("message", std::string("outputPath is null or empty"));
        result.set("data", std::string(""));
        result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
        std::string s = json::serialize(result);
        return api::StringBufferAllocator::instance().allocate(s);
    }

    converters::XlsxConverter conv;
    std::string resultJson = conv.fromJson(
        clientVarsJson,
        outputPath,
        dataDirectory ? dataDirectory : "",
        optionsJson ? optionsJson : "{}");

    return api::StringBufferAllocator::instance().allocate(resultJson);
}

} // extern "C"
