// =============================================================================
// src/ole2/compound_file_parser.h
//
// OLE2 复合文档顶层解析器
//
//   - Parse  解析整个 OLE2 文件
//   - GetStream  按名称获取流数据
//   - GetFileSystem  获取底层文件系统对象
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "compound_file_system.h"

namespace zq {
namespace office {
namespace ole2 {

/// OLE2 复合文档解析器
class CompoundFileParser {
public:
    CompoundFileParser() = default;

    /// 从文件路径加载并解析
    /// @param filePath  OLE2 文件路径（.xls/.ppt/.doc）
    /// @return 解析成功返回 true
    bool Parse(const std::string& filePath);

    /// 从内存缓冲加载并解析
    /// @param data  完整 OLE2 文件字节
    /// @return 解析成功返回 true
    bool ParseFromMemory(const std::vector<unsigned char>& data);

    /// 按名称打开流
    /// @param name  流名称，如 "Workbook" / "PowerPoint Document" / "1Table"
    /// @return 流对象
    Stream GetStream(const std::string& name);

    /// 列出根目录下所有子项
    std::vector<std::string> ListRootEntries() const;

    /// 获取底层文件系统
    CompoundFileSystem& fileSystem() { return fs_; }
    const CompoundFileSystem& fileSystem() const { return fs_; }

    /// 是否已解析
    bool isParsed() const { return parsed_; }

    /// 错误信息
    const std::string& error() const { return error_; }

private:
    bool parsed_ = false;
    std::string error_;
    CompoundFileSystem fs_;
};

} // namespace ole2
} // namespace office
} // namespace zq
