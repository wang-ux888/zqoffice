// =============================================================================
// src/utils/format_selector.h
//
// zq::office::FormatSelector 类
//   - XLS_EXT / PPT_EXT / WORD_EXT / TXT_EXT 静态集合
//   - isCompressedFile  检测 ZIP magic
// =============================================================================
#pragma once

#include <set>
#include <string>
#include <cstdint>

#include "zq/office/types.h"

namespace zq {
namespace office {

class FormatSelector {
public:
    /// Excel 文件扩展名集合：{".xlsx", ".xls", ".csv"}
    static const std::set<std::string> XLS_EXT;

    /// PowerPoint 文件扩展名集合：{".pptx", ".ppt"}
    static const std::set<std::string> PPT_EXT;

    /// Word 文件扩展名集合：{".docx", ".doc"}
    static const std::set<std::string> WORD_EXT;

    /// 文本文件扩展名集合：{".txt"}
    static const std::set<std::string> TXT_EXT;

    /// 判断文件头魔数是否为压缩文件（ZIP/OOXML 容器）
    /// @param magic  前 8 字节组成的小端 64 位整数
    /// @return true 表示是 ZIP 容器（OOXML：xlsx/pptx/docx）
    static bool isCompressedFile(std::int64_t magic);

    /// 判断文件头魔数是否为 OLE2 复合文档
    /// @param magic  前 8 字节组成的小端 64 位整数
    /// @return true 表示是 OLE2 容器（旧 xls/ppt/doc）
    static bool isOle2File(std::int64_t magic);

    /// 根据扩展名判断是否属于 Excel
    static bool isExcelExt(const std::string& ext);

    /// 根据扩展名判断是否属于 PowerPoint
    static bool isPptExt(const std::string& ext);

    /// 根据扩展名判断是否属于 Word
    static bool isWordExt(const std::string& ext);

    /// 根据扩展名判断是否为文本
    static bool isTxtExt(const std::string& ext);

    /// 根据文件路径扩展名 + 文件头魔数综合判断 FileFormat
    /// @param filePath  文件路径
    /// @return FileFormat 枚举值
    static FileFormat detectByFilePath(const std::string& filePath);

    /// 根据文件头魔数判断 FileFormat
    /// @param magic  前 8 字节小端整数
    /// @return FileFormat 枚举值（Xlsx/Pptx/Docx 走 ZIP 分支无法区分，返回 Xlsx）
    static FileFormat detectByMagic(std::int64_t magic);
};

} // namespace office
} // namespace zq
