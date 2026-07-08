// =============================================================================
// src/api/powerpoint_reader_api.cpp
//
// CreatePowerPointReader / DestroyPowerPointReader C API 绑定（Phase H-Part5 / H9）
//
// 与 ZQOffice 的 IReader 工厂模式对齐：
//   CreatePowerPointReader → 创建 PowerPointReader 实例并初始化
//   DestroyPowerPointReader → 销毁实例
// =============================================================================
#include "zq/office/office.h"

#include <string>

#include "api/converters/powerpoint_reader.h"

extern "C" {

// =============================================================================
// CreatePowerPointReader：创建并初始化 PowerPoint 流式读取器
// =============================================================================

ZQ_OFFICE_API void* ZQ_OFFICE_CALL
CreatePowerPointReader(const char* filePath) {
    if (!filePath || !*filePath) {
        return nullptr;
    }

    using namespace zq::office::converters;

    auto* reader = new PowerPointReader();
    std::string err;
    if (reader->initialize(filePath, &err) != 0) {
        // 初始化失败：销毁实例并返回 nullptr
        delete reader;
        return nullptr;
    }
    return reader;
}

// =============================================================================
// DestroyPowerPointReader：销毁 PowerPoint 流式读取器
// =============================================================================

ZQ_OFFICE_API void ZQ_OFFICE_CALL
DestroyPowerPointReader(void* reader) {
    if (!reader) return;  // 传入 nullptr 是安全空操作
    using namespace zq::office::converters;
    delete static_cast<PowerPointReader*>(reader);
}

} // extern "C"
