// =============================================================================
// src/ole2/compound_file_system.h
//
// OLE2 复合文档文件系统抽象
//
//   - OpenStream  按名称打开流
//   - ReadStream  读取流数据到缓冲
//   - 流可能走 FAT（>= miniStreamCutoff）或 MiniFAT（< miniStreamCutoff）
//   - MiniStream 存储在 Root Entry 的数据流中
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

#include "compound_file_header.h"
#include "fat_table.h"
#include "mini_fat_table.h"
#include "directory_entry.h"

namespace zq {
namespace office {
namespace ole2 {

/// 已读取的流
struct Stream {
    std::string name;
    std::vector<unsigned char> data;
    std::uint64_t size = 0;

    bool empty() const { return data.empty(); }
    const unsigned char* bytes() const { return data.data(); }
};

/// OLE2 文件系统抽象
class CompoundFileSystem {
public:
    CompoundFileSystem() = default;

    /// 从完整文件字节缓冲加载
    /// @param data  完整 OLE2 文件字节
    /// @return 加载成功返回 true
    bool Load(const std::vector<unsigned char>& data);

    /// 按路径打开流（路径分隔符 '/'）
    /// 例如 "Workbook" 或 "/Root Entry/Workbook"
    /// @param name  流名称（UTF-8）
    /// @return 流对象，失败返回空 Stream
    Stream OpenStream(const std::string& name);

    /// 按目录项 ID 打开流
    Stream OpenStreamById(std::uint32_t entryId);

    /// 列出根目录下所有子项名称
    std::vector<std::string> ListRootEntries() const;

    /// 列出指定 storage 下的所有子项名称
    std::vector<std::string> ListChildren(std::uint32_t parentId) const;

    /// 获取目录项表
    const DirectoryEntryTable& directory() const { return directory_; }

    /// 获取文件头
    const CompoundFileHeader& header() const { return header_; }

    /// 是否已加载
    bool isLoaded() const { return loaded_; }

    /// 错误信息
    const std::string& error() const { return error_; }

private:
    /// 读取 DIFAT 链，收集所有 FAT 扇区号
    bool ReadDifat();

    /// 从 FAT 扇区构建 FATTable
    bool BuildFatTable();

    /// 从 MiniFAT 链构建 MiniFATTable
    bool BuildMiniFatTable();

    /// 从目录链构建 DirectoryEntryTable
    bool BuildDirectory();

    /// 读取 Root Entry 的 MiniStream 数据
    bool BuildMiniStream();

    /// 读取 FAT 链上的所有扇区数据
    std::vector<unsigned char> ReadSectorChain(
        const std::vector<std::uint32_t>& chain,
        std::uint64_t maxSize);

    /// 读取 MiniFAT 链上的所有 mini 扇区数据
    std::vector<unsigned char> ReadMiniSectorChain(
        const std::vector<std::uint32_t>& chain,
        std::uint64_t maxSize);

    /// 按名称递归查找目录项（路径分隔符 '/'）
    /// @return 找到的 entryId，未找到返回 kNoStream
    std::uint32_t FindEntryByPath(const std::string& path) const;

    /// 递归遍历目录树查找名称
    std::uint32_t FindEntryRecursive(std::uint32_t parentId,
                                      const std::vector<std::string>& segments,
                                      std::size_t segIdx) const;

    /// 在 storage 的红黑树中查找指定名称的子项
    std::uint32_t FindInStorage(std::uint32_t storageId,
                                 const std::string& name) const;

    /// 红黑树中序遍历查找
    std::uint32_t RbTreeFind(std::uint32_t rootId,
                              const std::u16string& targetName) const;

    /// UTF-16 名称大小写不敏感比较
    /// 返回 <0 表示 a < b，0 表示相等，>0 表示 a > b
    static int CompareNameIgnoreCase(const std::u16string& a,
                                      const std::u16string& b);

    /// 获取扇区在文件中的字节偏移
    std::size_t SectorOffset(std::uint32_t sector) const {
        return CompoundFileHeader::kHeaderSize + sector * header_.sectorSize();
    }

private:
    bool loaded_ = false;
    std::string error_;
    std::vector<unsigned char> fileData_;
    CompoundFileHeader header_;
    FATTable fat_;
    MiniFATTable miniFat_;
    DirectoryEntryTable directory_;
    std::vector<unsigned char> miniStream_; // Root Entry 的 MiniStream 数据
    std::vector<std::uint32_t> difatSectors_; // 所有 FAT 扇区号
};

} // namespace ole2
} // namespace office
} // namespace zq
