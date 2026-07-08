// =============================================================================
// src/ooxml/zip_writer.h
//
// OOXML ZIP 容器写入器
//
//   OOXML 文件（.xlsx/.pptx/.docx）本质是 ZIP 压缩包。
//   ZipWriter 提供：
//     - 创建/打开输出 ZIP 文件
//     - 添加文件条目（按名称 + 内容）
//     - 关闭并 finalize（写出中央目录 + EOCD）
//
//   基于 miniz 单文件库（mz_zip_writer_* API）：
//     - mz_zip_writer_init_file：初始化写入器（绑定文件句柄）
//     - mz_zip_writer_add_mem：添加内存中的条目
//     - mz_zip_writer_finalize_archive：完成归档（写中央目录 + EOCD）
//
//   注意：finalize 后 archive 句柄自动关闭，无需再调用 mz_zip_writer_end
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {

/// OOXML ZIP 容器写入器
///
/// 用法：
///   ZipWriter w;
///   if (!w.open("out.xlsx")) { ... }
///   w.addFile("[Content_Types].xml", contentTypesXml);
///   w.addFile("xl/workbook.xml", workbookXml);
///   ...
///   w.close();  // finalize 并写出
class ZipWriter {
public:
    ZipWriter();
    ~ZipWriter();

    ZipWriter(const ZipWriter&) = delete;
    ZipWriter& operator=(const ZipWriter&) = delete;

    // -----------------------------------------------------------------------
    // 打开 / 关闭
    // -----------------------------------------------------------------------

    /// 打开输出 ZIP 文件（覆盖已存在文件）
    /// @param path  输出文件路径
    /// @return 成功返回 true
    bool open(const std::string& path);

    /// 添加文件条目（字符串内容）
    /// @param name     条目名称（如 "xl/workbook.xml"）
    /// @param content  文件内容
    /// @return 成功返回 true
    bool addFile(const std::string& name, const std::string& content);

    /// 添加文件条目（二进制内容）
    /// @param name  条目名称
    /// @param data  数据指针
    /// @param size  字节数
    /// @return 成功返回 true
    bool addFile(const std::string& name, const void* data, std::size_t size);

    /// 关闭并 finalize 归档
    /// @return 成功返回 true
    /// @note close 后不可再 addFile；可安全多次调用
    bool close();

    /// 是否已打开
    bool isOpen() const { return opened_; }

    /// 最后一次错误信息
    const std::string& error() const { return error_; }

private:
    void* archive_ = nullptr;   // mz_zip_archive*（内部类型）
    bool opened_ = false;
    std::string error_;
};

} // namespace ooxml
} // namespace office
} // namespace zq
