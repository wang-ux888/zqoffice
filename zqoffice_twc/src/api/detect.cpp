// =============================================================================
// src/api/detect.cpp
//
// 实现 ZQOfficeDetectFileFormat
// Phase A：仅按扩展名 + 文件头魔数检测
// 后续 Phase C 完成后：可打开 ZIP 容器读取 [Content_Types].xml 精确区分
// =============================================================================
#include "zq/office/office.h"
#include "zq/office/types.h"
#include "../utils/format_selector.h"
#include "string_buffer.h"

#include <fstream>
#include <vector>
#include <string>

extern "C" {

ZQ_OFFICE_API int ZQ_OFFICE_CALL
ZQOfficeDetectFileFormat(const char* filePath) {
    if (!filePath || !*filePath) {
        return static_cast<int>(zq::office::FileFormat::Unknown);
    }
    std::string path(filePath);
    return static_cast<int>(zq::office::FormatSelector::detectByFilePath(path));
}

} // extern "C"
