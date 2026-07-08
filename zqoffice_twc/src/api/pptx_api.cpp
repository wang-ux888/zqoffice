// =============================================================================
// src/api/pptx_api.cpp
//
// PptxToZQSlide / ZQSlideToPptx C API 绑定（Phase H-Part3）
//
// 将 C API 调用委托给 PptxConverter
// =============================================================================
#include "zq/office/office.h"
#include "zq/office/types.h"
#include "zq/office/version.h"
#include "string_buffer.h"

#include <string>

#include "api/converters/pptx_converter.h"
#include "api/json/json.h"

extern "C" {

// =============================================================================
// PptxToZQSlide（Phase H6）：pptx 文件 → ZQ Slide JSON
// =============================================================================

ZQ_OFFICE_API const char* ZQ_OFFICE_CALL
PptxToZQSlide(const char* filePath,
                const char* dataDirectory,
                const char* reserved,
                const char* optionsJson) {
    using namespace zq::office;

    if (!filePath || !*filePath) {
        json::JsonValue result = json::JsonValue::object();
        result.set("code", static_cast<std::int64_t>(ErrorCode::InvalidArgument));
        result.set("message", std::string("filePath is null or empty"));
        result.set("data", std::string(""));
        result.set("version", std::string(ZQ_OFFICE_VERSION_STRING));
        std::string s = json::serialize(result);
        return api::StringBufferAllocator::instance().allocate(s);
    }

    (void)reserved;  // 预留参数

    converters::PptxConverter conv;
    std::string resultJson = conv.toJson(
        filePath,
        dataDirectory ? dataDirectory : "",
        optionsJson ? optionsJson : "{}");

    return api::StringBufferAllocator::instance().allocate(resultJson);
}

// =============================================================================
// ZQSlideToPptx（Phase H7）：ZQ Slide JSON → pptx 文件
// =============================================================================

ZQ_OFFICE_API const char* ZQ_OFFICE_CALL
ZQSlideToPptx(const char* clientVarsJson,
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

    converters::PptxConverter conv;
    std::string resultJson = conv.fromJson(
        clientVarsJson,
        outputPath,
        dataDirectory ? dataDirectory : "",
        optionsJson ? optionsJson : "{}");

    return api::StringBufferAllocator::instance().allocate(resultJson);
}

} // extern "C"
