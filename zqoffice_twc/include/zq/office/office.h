// =============================================================================
// zq/office/office.h
//
// ZQOffice 顶层 C API 声明
//
// 调用约定：所有 C API 返回的 const char* 字符串均由 ZQOffice 内部缓冲区管理，
//         调用方使用完毕后必须通过 FreeStringBuffer 释放。
// =============================================================================
#pragma once

#include "export.h"
#include "types.h"
#include "version.h"

// ============================================================================
// 引擎生命周期
// ============================================================================

/// 初始化 ZQOffice 引擎
/// @param workingDirectory  工作目录路径（用于临时文件、缓存）
///                          传 nullptr 表示使用系统临时目录
/// @param mode              初始化模式，0=完整 1=最小 2=调试
///                          （对应 InitMode 枚举）

ZQ_OFFICE_C_API void ZQ_OFFICE_CALL
ZQOfficeInitialize(const char* workingDirectory, int mode);

/// 反初始化引擎，释放所有内部资源

ZQ_OFFICE_C_API void ZQ_OFFICE_CALL
ZQOfficeUninitialize();

// ============================================================================
// 文件格式检测
// ============================================================================

/// 检测给定文件路径的 Office 文件格式
/// @param filePath  文件绝对路径
/// @return FileFormat 枚举值（int），见 types.h

ZQ_OFFICE_C_API int ZQ_OFFICE_CALL
ZQOfficeDetectFileFormat(const char* filePath);

// ============================================================================
// Excel：xlsx ↔ ZQ Sheet JSON
// ============================================================================

/// xlsx 文件 → ZQ Sheet JSON
/// @param filePath        xlsx 文件路径
/// @param dataDirectory   数据目录（存放图片等附件）
/// @param reserved        预留参数，传 nullptr
/// @param optionsJson     转换选项 JSON（可传 nullptr 表示默认选项）
/// @return 转换结果 JSON 字符串（结构见 ExportResult），
///         调用方需通过 FreeStringBuffer 释放
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
XlsxToZQSheet(const char* filePath,
                const char* dataDirectory,
                const char* reserved,
                const char* optionsJson);

/// ZQ Sheet JSON → xlsx 文件
/// @param clientVarsJson  ZQ Sheet 的 clientVars JSON
/// @param outputPath      输出 xlsx 文件路径
/// @param dataDirectory   数据目录
/// @param reserved        预留参数
/// @param optionsJson     转换选项 JSON
/// @return 转换结果 JSON 字符串
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
ZQSheetToXlsx(const char* clientVarsJson,
                const char* outputPath,
                const char* dataDirectory,
                const char* reserved,
                const char* optionsJson);

// ============================================================================
// PowerPoint：pptx ↔ ZQ Slide JSON
// ============================================================================

/// pptx 文件 → ZQ Slide JSON
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
PptxToZQSlide(const char* filePath,
                const char* dataDirectory,
                const char* reserved,
                const char* optionsJson);

/// ZQ Slide JSON → pptx 文件
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
ZQSlideToPptx(const char* clientVarsJson,
                const char* outputPath,
                const char* dataDirectory,
                const char* reserved,
                const char* optionsJson);

// ============================================================================
// Word：docx ↔ ZQ Doc JSON
// ============================================================================

/// docx 文件 → ZQ Doc JSON
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
DocxToZQDoc(const char* filePath,
              const char* dataDirectory,
              const char* reserved,
              const char* optionsJson);

/// ZQ Doc JSON → docx 文件
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
ZQDocToDocx(const char* clientVarsJson,
              const char* outputPath,
              const char* dataDirectory,
              const char* reserved,
              const char* optionsJson);

// ============================================================================
// 通用导入/导出
// ============================================================================

/// 通用 ZQ 导出
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
ZQOfficeExport(const char* sourcePath,
                    const char* outputPath,
                    const char* optionsJson);

/// 通用 ZQ 导入
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
ZQOfficeImport(const char* clientVarsJson,
                    const char* outputPath,
                    const char* optionsJson);

/// 通用文件导出
ZQ_OFFICE_C_API const char* ZQ_OFFICE_CALL
ZQExportFile(const char* sourcePath,
               const char* outputPath,
               const char* formatName,
               const char* optionsJson);

// ============================================================================
// PowerPoint 流式读取器
// ============================================================================

/// 创建 PowerPoint 流式读取器
/// @param filePath  ppt/pptx 文件路径
/// @return 读取器不透明句柄，失败返回 nullptr
///         使用完毕必须通过 DestroyPowerPointReader 销毁

ZQ_OFFICE_C_API void* ZQ_OFFICE_CALL
CreatePowerPointReader(const char* filePath);

/// 销毁 PowerPoint 流式读取器
/// @param reader  CreatePowerPointReader 返回的句柄

ZQ_OFFICE_C_API void ZQ_OFFICE_CALL
DestroyPowerPointReader(void* reader);

// ============================================================================
// 内存管理
// ============================================================================

/// 释放由 ZQOffice C API 返回的字符串缓冲区
/// @param buffer  之前由 XlsxToZQSheet / PptxToZQSlide 等函数返回的 const char*
/// @note 传入 nullptr 是安全空操作

ZQ_OFFICE_C_API void ZQ_OFFICE_CALL
FreeStringBuffer(const char* buffer);

// ============================================================================
// C++ 便捷包装（仅 C++ 模式可用）
// ============================================================================
#if defined(__cplusplus)

#include <string>
#include <memory>

namespace zq {
namespace office {

/// C++ RAII 包装：自动释放返回的字符串缓冲
class StringBuffer {
public:
    StringBuffer() = default;
    explicit StringBuffer(const char* buf) : buf_(buf) {}
    ~StringBuffer() { reset(); }

    StringBuffer(StringBuffer&& o) noexcept : buf_(o.buf_) { o.buf_ = nullptr; }
    StringBuffer& operator=(StringBuffer&& o) noexcept {
        if (this != &o) { reset(); buf_ = o.buf_; o.buf_ = nullptr; }
        return *this;
    }
    StringBuffer(const StringBuffer&) = delete;
    StringBuffer& operator=(const StringBuffer&) = delete;

    const char* get() const { return buf_; }
    bool valid() const { return buf_ != nullptr; }
    void reset() {
        if (buf_) {
            FreeStringBuffer(buf_);
            buf_ = nullptr;
        }
    }
    std::string str() const { return buf_ ? std::string(buf_) : std::string(); }
private:
    const char* buf_ = nullptr;
};

} // namespace office
} // namespace zq

#endif // __cplusplus
