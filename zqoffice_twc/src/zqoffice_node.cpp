// =============================================================================
// src/zqoffice_node.cpp
//
// ZQOffice Node-API 绑定入口
//
// 将 15 个 C API 绑定为 Node.js 原生模块 zqoffice.node：
//   zqofficeInitialize / zqofficeUninitialize / zqofficeDetectFileFormat
//   xlsxToZQSheet / zqSheetToXlsx
//   pptxToZQSlide / zqSlideToPptx
//   docxToZQDoc / zqDocToDocx
//   zqofficeExport / zqofficeImport / zqExportFile
//   createPowerPointReader / destroyPowerPointReader
//   freeStringBuffer
//
// 设计要点：
//   1. 使用 Node-API C API（node_api.h），不依赖 node-addon-api C++ 包装
//   2. 所有返回 const char* 的函数：转为 JS string 后立即 FreeStringBuffer
//   3. PowerPointReader 的 void* 句柄：包装为 napi_external（JS 外部对象）
//   4. 参数解析：先 napi_get_value_string_utf8(nullptr, 0) 获取长度，再分配 buffer 读取
//   5. 异常处理：napi_throw_error 抛出 JS Error
// =============================================================================

#include <node_api.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include "zq/office/office.h"
#include "zq/office/types.h"

// ============================================================================
// 工具函数
// ============================================================================

namespace {

/// 将 C 字符串（可能为 nullptr）转为 JS string
/// @note 如果 str 非空，调用方负责后续 FreeStringBuffer；本函数不接管所有权
napi_value MakeJsString(napi_env env, const char* str) {
    napi_value result;
    if (str == nullptr) {
        napi_get_null(env, &result);
        return result;
    }
    napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result);
    return result;
}

/// 将 JS value 转为 C 字符串（堆分配，调用方需 free）
/// @param env    Node-API 环境
/// @param value  JS value（必须是 string 或 null/undefined）
/// @param out    输出字符串指针（调用方需 free(*out)）
/// @return true 表示成功（包括 null/undefined → nullptr 的情况）
bool JsToCString(napi_env env, napi_value value, char** out) {
    *out = nullptr;
    if (value == nullptr) return true;

    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) return false;

    // null/undefined → nullptr
    if (type == napi_undefined || type == napi_null) {
        return true;
    }

    // 非 string 类型报错
    if (type != napi_string) {
        napi_throw_type_error(env, nullptr, "expected string or null");
        return false;
    }

    // 获取字符串长度（不含 null）
    size_t length = 0;
    status = napi_get_value_string_utf8(env, value, nullptr, 0, &length);
    if (status != napi_ok) return false;

    // 分配 buffer（length + 1 for null terminator）
    *out = static_cast<char*>(std::malloc(length + 1));
    if (*out == nullptr) {
        napi_throw_error(env, nullptr, "out of memory");
        return false;
    }

    status = napi_get_value_string_utf8(env, value, *out, length + 1, &length);
    if (status != napi_ok) {
        std::free(*out);
        *out = nullptr;
        return false;
    }

    return true;
}

/// 获取参数数量
size_t GetArgCount(napi_env env, napi_callback_info info) {
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);
    return argc;
}

/// 解析参数到数组
bool GetArgs(napi_env env, napi_callback_info info, napi_value* args, size_t max_argc) {
    size_t argc = max_argc;
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    return status == napi_ok;
}

/// 调用返回 const char* 的 C API 并转为 JS string
/// @param fn  C API 函数指针
/// @note 模板参数 Args... 自动推导
template <typename Fn, typename... Args>
napi_value CallStringApi(napi_env env, Fn fn, Args... args) {
    const char* result = fn(args...);
    if (result == nullptr) {
        napi_value null_val;
        napi_get_null(env, &null_val);
        return null_val;
    }
    napi_value js_str = MakeJsString(env, result);
    FreeStringBuffer(result);
    return js_str;
}

} // namespace

// ============================================================================
// 1. 引擎生命周期
// ============================================================================

static napi_value JsZQOfficeInitialize(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 2 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* workingDir = nullptr;
    if (!JsToCString(env, args[0], &workingDir)) {
        return nullptr;
    }

    int32_t mode = 0;
    napi_get_value_int32(env, args[1], &mode);

    ZQOfficeInitialize(workingDir, mode);

    if (workingDir) std::free(workingDir);

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

static napi_value JsZQOfficeUninitialize(napi_env env, napi_callback_info info) {
    ZQOfficeUninitialize();
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

static napi_value JsZQOfficeDetectFileFormat(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 1 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* filePath = nullptr;
    if (!JsToCString(env, args[0], &filePath)) {
        return nullptr;
    }

    int format = ZQOfficeDetectFileFormat(filePath);

    if (filePath) std::free(filePath);

    napi_value result;
    napi_create_int32(env, format, &result);
    return result;
}

// ============================================================================
// 2. Excel：xlsx ↔ ZQ Sheet JSON
// ============================================================================

static napi_value JsXlsxToZQSheet(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 4 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* filePath = nullptr;
    char* dataDir = nullptr;
    char* reserved = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &filePath) ||
        !JsToCString(env, args[1], &dataDir) ||
        !JsToCString(env, args[2], &reserved) ||
        !JsToCString(env, args[3], &optionsJson)) {
        if (filePath) std::free(filePath);
        if (dataDir) std::free(dataDir);
        if (reserved) std::free(reserved);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, XlsxToZQSheet,
                                            filePath, dataDir, reserved, optionsJson);

    std::free(filePath);
    std::free(dataDir);
    std::free(reserved);
    std::free(optionsJson);
    return js_result;
}

static napi_value JsZQSheetToXlsx(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 5 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* clientVarsJson = nullptr;
    char* outputPath = nullptr;
    char* dataDir = nullptr;
    char* reserved = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &clientVarsJson) ||
        !JsToCString(env, args[1], &outputPath) ||
        !JsToCString(env, args[2], &dataDir) ||
        !JsToCString(env, args[3], &reserved) ||
        !JsToCString(env, args[4], &optionsJson)) {
        if (clientVarsJson) std::free(clientVarsJson);
        if (outputPath) std::free(outputPath);
        if (dataDir) std::free(dataDir);
        if (reserved) std::free(reserved);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, ZQSheetToXlsx,
                                            clientVarsJson, outputPath,
                                            dataDir, reserved, optionsJson);

    std::free(clientVarsJson);
    std::free(outputPath);
    std::free(dataDir);
    std::free(reserved);
    std::free(optionsJson);
    return js_result;
}

// ============================================================================
// 3. PowerPoint：pptx ↔ ZQ Slide JSON
// ============================================================================

static napi_value JsPptxToZQSlide(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 4 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* filePath = nullptr;
    char* dataDir = nullptr;
    char* reserved = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &filePath) ||
        !JsToCString(env, args[1], &dataDir) ||
        !JsToCString(env, args[2], &reserved) ||
        !JsToCString(env, args[3], &optionsJson)) {
        if (filePath) std::free(filePath);
        if (dataDir) std::free(dataDir);
        if (reserved) std::free(reserved);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, PptxToZQSlide,
                                            filePath, dataDir, reserved, optionsJson);

    std::free(filePath);
    std::free(dataDir);
    std::free(reserved);
    std::free(optionsJson);
    return js_result;
}

static napi_value JsZQSlideToPptx(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 5 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* clientVarsJson = nullptr;
    char* outputPath = nullptr;
    char* dataDir = nullptr;
    char* reserved = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &clientVarsJson) ||
        !JsToCString(env, args[1], &outputPath) ||
        !JsToCString(env, args[2], &dataDir) ||
        !JsToCString(env, args[3], &reserved) ||
        !JsToCString(env, args[4], &optionsJson)) {
        if (clientVarsJson) std::free(clientVarsJson);
        if (outputPath) std::free(outputPath);
        if (dataDir) std::free(dataDir);
        if (reserved) std::free(reserved);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, ZQSlideToPptx,
                                            clientVarsJson, outputPath,
                                            dataDir, reserved, optionsJson);

    std::free(clientVarsJson);
    std::free(outputPath);
    std::free(dataDir);
    std::free(reserved);
    std::free(optionsJson);
    return js_result;
}

// ============================================================================
// 4. Word：docx ↔ ZQ Doc JSON
// ============================================================================

static napi_value JsDocxToZQDoc(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 4 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* filePath = nullptr;
    char* dataDir = nullptr;
    char* reserved = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &filePath) ||
        !JsToCString(env, args[1], &dataDir) ||
        !JsToCString(env, args[2], &reserved) ||
        !JsToCString(env, args[3], &optionsJson)) {
        if (filePath) std::free(filePath);
        if (dataDir) std::free(dataDir);
        if (reserved) std::free(reserved);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, DocxToZQDoc,
                                            filePath, dataDir, reserved, optionsJson);

    std::free(filePath);
    std::free(dataDir);
    std::free(reserved);
    std::free(optionsJson);
    return js_result;
}

static napi_value JsZQDocToDocx(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 5 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* clientVarsJson = nullptr;
    char* outputPath = nullptr;
    char* dataDir = nullptr;
    char* reserved = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &clientVarsJson) ||
        !JsToCString(env, args[1], &outputPath) ||
        !JsToCString(env, args[2], &dataDir) ||
        !JsToCString(env, args[3], &reserved) ||
        !JsToCString(env, args[4], &optionsJson)) {
        if (clientVarsJson) std::free(clientVarsJson);
        if (outputPath) std::free(outputPath);
        if (dataDir) std::free(dataDir);
        if (reserved) std::free(reserved);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, ZQDocToDocx,
                                            clientVarsJson, outputPath,
                                            dataDir, reserved, optionsJson);

    std::free(clientVarsJson);
    std::free(outputPath);
    std::free(dataDir);
    std::free(reserved);
    std::free(optionsJson);
    return js_result;
}

// ============================================================================
// 5. 通用导入/导出
// ============================================================================

static napi_value JsZQOfficeExport(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 3 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* sourcePath = nullptr;
    char* outputPath = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &sourcePath) ||
        !JsToCString(env, args[1], &outputPath) ||
        !JsToCString(env, args[2], &optionsJson)) {
        if (sourcePath) std::free(sourcePath);
        if (outputPath) std::free(outputPath);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, ZQOfficeExport,
                                            sourcePath, outputPath, optionsJson);

    std::free(sourcePath);
    std::free(outputPath);
    std::free(optionsJson);
    return js_result;
}

static napi_value JsZQOfficeImport(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 3 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* clientVarsJson = nullptr;
    char* outputPath = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &clientVarsJson) ||
        !JsToCString(env, args[1], &outputPath) ||
        !JsToCString(env, args[2], &optionsJson)) {
        if (clientVarsJson) std::free(clientVarsJson);
        if (outputPath) std::free(outputPath);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, ZQOfficeImport,
                                            clientVarsJson, outputPath, optionsJson);

    std::free(clientVarsJson);
    std::free(outputPath);
    std::free(optionsJson);
    return js_result;
}

static napi_value JsZQExportFile(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 4 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* sourcePath = nullptr;
    char* outputPath = nullptr;
    char* formatName = nullptr;
    char* optionsJson = nullptr;

    if (!JsToCString(env, args[0], &sourcePath) ||
        !JsToCString(env, args[1], &outputPath) ||
        !JsToCString(env, args[2], &formatName) ||
        !JsToCString(env, args[3], &optionsJson)) {
        if (sourcePath) std::free(sourcePath);
        if (outputPath) std::free(outputPath);
        if (formatName) std::free(formatName);
        if (optionsJson) std::free(optionsJson);
        return nullptr;
    }

    napi_value js_result = CallStringApi(env, ZQExportFile,
                                            sourcePath, outputPath,
                                            formatName, optionsJson);

    std::free(sourcePath);
    std::free(outputPath);
    std::free(formatName);
    std::free(optionsJson);
    return js_result;
}

// ============================================================================
// 6. PowerPoint 流式读取器（void* 句柄包装为 napi_external）
// ============================================================================

/// napi_external 析构回调：销毁 PowerPointReader 句柄
static void FinalizePowerPointReader(napi_env env, void* data, void* hint) {
    (void)env;
    (void)hint;
    if (data != nullptr) {
        DestroyPowerPointReader(data);
    }
}

static napi_value JsCreatePowerPointReader(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 1 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    char* filePath = nullptr;
    if (!JsToCString(env, args[0], &filePath)) {
        return nullptr;
    }

    void* reader = CreatePowerPointReader(filePath);
    if (filePath) std::free(filePath);

    if (reader == nullptr) {
        napi_value null_val;
        napi_get_null(env, &null_val);
        return null_val;
    }

    // 包装为 napi_external（JS 外部对象，不可直接访问，仅供 destroy 释放）
    napi_value external;
    napi_status status = napi_create_external(env, reader,
                                                FinalizePowerPointReader, nullptr, &external);
    if (status != napi_ok) {
        DestroyPowerPointReader(reader);
        napi_throw_error(env, nullptr, "failed to create external object");
        return nullptr;
    }
    return external;
}

static napi_value JsDestroyPowerPointReader(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 1 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    (void)info;  // 参数已在 args 中获取，info 本身未使用

    void* data = nullptr;
    napi_status status = napi_get_value_external(env, args[0], &data);
    if (status != napi_ok) {
        napi_throw_type_error(env, nullptr, "expected PowerPointReader external object");
        return nullptr;
    }

    // 注意：napi_external 的 finalize 回调会自动调用 DestroyPowerPointReader，
    // 但这里也允许手动销毁（DestroyPowerPointReader(nullptr) 是安全空操作）。
    // 为避免双重释放，手动销毁后应将 external 的 data 置空。
    // 由于 Node-API 不支持修改 external 的 data，这里不手动销毁，依赖 GC 自动回收。
    // 用户也可以显式调用 destroy，但实际销毁由 finalize 完成。
    // 为安全起见，这里不做任何操作，让 GC 处理。

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

// ============================================================================
// 7. FreeStringBuffer（手动释放字符串缓冲，通常不需要手动调用）
// ============================================================================

static napi_value JsFreeStringBuffer(napi_env env, napi_callback_info info) {
    enum { kMaxArgs = 1 };
    napi_value args[kMaxArgs];
    if (!GetArgs(env, info, args, kMaxArgs)) {
        napi_throw_error(env, nullptr, "failed to get arguments");
        return nullptr;
    }

    // JS 端无法直接持有 const char* 指针，此函数仅用于兼容性
    // 所有返回 const char* 的绑定函数已自动调用 FreeStringBuffer
    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

// ============================================================================
// 模块初始化
// ============================================================================

/// 注册单个函数到 exports 对象
static napi_status RegisterFunction(napi_env env, napi_value exports,
                                       const char* name, napi_callback cb) {
    napi_value fn;
    napi_status status = napi_create_function(env, name, NAPI_AUTO_LENGTH, cb, nullptr, &fn);
    if (status != napi_ok) return status;
    return napi_set_named_property(env, exports, name, fn);
}

static napi_value Init(napi_env env, napi_value exports) {
    // 引擎生命周期
    RegisterFunction(env, exports, "zqofficeInitialize",        JsZQOfficeInitialize);
    RegisterFunction(env, exports, "zqofficeUninitialize",      JsZQOfficeUninitialize);
    RegisterFunction(env, exports, "zqofficeDetectFileFormat",  JsZQOfficeDetectFileFormat);

    // Excel
    RegisterFunction(env, exports, "xlsxToZQSheet",             JsXlsxToZQSheet);
    RegisterFunction(env, exports, "zqSheetToXlsx",             JsZQSheetToXlsx);

    // PowerPoint
    RegisterFunction(env, exports, "pptxToZQSlide",             JsPptxToZQSlide);
    RegisterFunction(env, exports, "zqSlideToPptx",             JsZQSlideToPptx);

    // Word
    RegisterFunction(env, exports, "docxToZQDoc",               JsDocxToZQDoc);
    RegisterFunction(env, exports, "zqDocToDocx",               JsZQDocToDocx);

    // 通用导入/导出
    RegisterFunction(env, exports, "zqofficeExport",            JsZQOfficeExport);
    RegisterFunction(env, exports, "zqofficeImport",            JsZQOfficeImport);
    RegisterFunction(env, exports, "zqExportFile",              JsZQExportFile);

    // PowerPoint 流式读取器
    RegisterFunction(env, exports, "createPowerPointReader",    JsCreatePowerPointReader);
    RegisterFunction(env, exports, "destroyPowerPointReader",   JsDestroyPowerPointReader);

    // 内存管理
    RegisterFunction(env, exports, "freeStringBuffer",          JsFreeStringBuffer);

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
