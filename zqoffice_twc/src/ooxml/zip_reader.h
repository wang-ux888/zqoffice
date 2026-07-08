// =============================================================================
// src/ooxml/zip_reader.h
//
// OOXML ZIP 容器读取器
//
//   OOXML 文件（.xlsx/.pptx/.docx）本质是 ZIP 压缩包，内部包含 XML 文件和资源。
//   ZipReader 提供：
//     - 从文件路径或内存缓冲打开 ZIP
//     - 列出条目
//     - 按名称读取条目内容（解压后）
//
//   基于 miniz 单文件库（https://github.com/richgel999/miniz）：
//     - 自带 deflate/inflate，不依赖系统 zlib
//     - MIT 协议
//     - 单文件集成，CMake 配置简单
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {

/// ZIP 条目信息
struct ZipEntry {
    std::string name;                       ///< 条目名称（如 "xl/workbook.xml"）
    std::uint64_t uncompressedSize = 0;     ///< 解压后大小（字节）
    std::uint64_t compressedSize = 0;       ///< 压缩后大小（字节）
    std::uint32_t crc = 0;                  ///< CRC32 校验值（避免与 miniz 的 crc32 宏冲突）
    bool isDirectory = false;               ///< 是否为目录条目

    /// 是否为文件条目
    bool isFile() const { return !isDirectory; }
};

/// OOXML ZIP 容器读取器
///
///   - 一次性加载整个 ZIP 索引到内存
///   - 按需解压单个条目
///   - 线程不安全（单线程读取使用）
class ZipReader {
public:
    ZipReader();
    ~ZipReader();

    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;

    // -----------------------------------------------------------------------
    // 打开 / 关闭
    // -----------------------------------------------------------------------

    /// 从文件路径打开 ZIP
    /// @param filePath  ZIP 文件路径（如 "test.xlsx"）
    /// @return 打开成功返回 true
    bool open(const std::string& filePath);

    /// 从内存缓冲打开 ZIP
    /// @param data  完整 ZIP 文件字节
    /// @return 打开成功返回 true
    bool openFromMemory(const std::vector<unsigned char>& data);

    /// 从内存缓冲打开 ZIP（指针 + 大小）
    /// @param data  数据指针
    /// @param size  字节数
    /// @return 打开成功返回 true
    /// @note 调用方需保证 data 在 ZipReader 生命周期内有效
    bool openFromMemory(const unsigned char* data, std::size_t size);

    /// 关闭 ZIP 归档
    void close();

    /// 是否已打开
    bool isOpen() const;

    // -----------------------------------------------------------------------
    // 条目访问
    // -----------------------------------------------------------------------

    /// 列出所有条目
    const std::vector<ZipEntry>& entries() const;

    /// 检查条目是否存在
    /// @param name  条目名称（如 "xl/workbook.xml"）
    bool hasEntry(const std::string& name) const;

    /// 读取条目内容（解压后）
    /// @param name  条目名称
    /// @return 解压后的字节流，失败返回空 vector
    std::vector<unsigned char> readEntry(const std::string& name) const;

    /// 读取条目为 UTF-8 字符串
    /// @param name  条目名称
    /// @return 解压后的字符串，失败返回空字符串
    std::string readEntryText(const std::string& name) const;

    /// 条目数量
    std::size_t entryCount() const;

    /// 按索引读取条目内容
    /// @param index  条目索引（0-based）
    /// @return 解压后的字节流，失败返回空 vector
    std::vector<unsigned char> readEntryAt(std::size_t index) const;

    /// 按索引获取条目信息
    /// @param index  条目索引
    /// @return 条目信息指针，越界返回 nullptr
    const ZipEntry* entryAt(std::size_t index) const;

    // -----------------------------------------------------------------------
    // 错误信息
    // -----------------------------------------------------------------------

    /// 最后一次错误信息
    const std::string& error() const { return error_; }

private:
    /// 加载所有条目索引
    bool loadEntries_();

    /// 查找条目索引
    /// @return 找到返回索引，未找到返回 SIZE_MAX
    std::size_t findEntry_(const std::string& name) const;

    void* archive_ = nullptr;                  // mz_zip_archive*（内部类型）
    std::vector<ZipEntry> entries_;
    mutable std::string error_;                // mutable：允许 const 方法设置错误信息

    // 当通过 openFromMemory(data, size) 打开时，持有数据副本
    // 当通过 openFromMemory(vector) 打开时，仅持有引用（调用方保证生命周期）
    std::vector<unsigned char> ownedData_;
    const unsigned char* externalData_ = nullptr;
    std::size_t externalSize_ = 0;
};

} // namespace ooxml
} // namespace office
} // namespace zq
