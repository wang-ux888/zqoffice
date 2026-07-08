// =============================================================================
// src/ole2/directory_entry.h
//
// OLE2 目录项（[MS-CFB] 2.6.1 节，每个 128 字节）
//
//   - Object Type: 0=Unknown, 1=Storage, 2=Stream, 5=Root Storage
//   - 红黑树组织：LeftSibling / RightSibling / Child
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "compound_file_header.h"

namespace zq {
namespace office {
namespace ole2 {

/// 目录项类型
enum class ObjectType : std::uint8_t {
    Unknown     = 0x00,
    Storage     = 0x01,
    Stream      = 0x02,
    RootStorage = 0x05,
    LockBytes   = 0x03,
    Property    = 0x04,
};

/// 红黑树节点颜色
enum class TreeNodeColor : std::uint8_t {
    Red   = 0x00,
    Black = 0x01,
};

/// 单个目录项（128 字节）
class DirectoryEntry {
public:
    static constexpr std::size_t kEntrySize = 128;
    static constexpr std::uint32_t kNoStream = 0xFFFFFFFFu;

    DirectoryEntry() = default;

    /// 从 128 字节缓冲读取
    bool Read(const unsigned char* buf, std::size_t size);

    // -----------------------------------------------------------------------
    // 访问器
    // -----------------------------------------------------------------------

    /// UTF-16 名称（去除末尾 null）
    const std::u16string& name() const { return name_; }

    /// UTF-8 名称（便于日志与匹配）
    std::string utf8Name() const;

    /// 名称字节数（含 null 终止）
    std::uint16_t nameLength() const { return nameLength_; }

    ObjectType objectType() const { return objectType_; }
    TreeNodeColor color() const { return color_; }

    std::uint32_t leftSiblingId() const { return leftSiblingId_; }
    std::uint32_t rightSiblingId() const { return rightSiblingId_; }
    std::uint32_t childId() const { return childId_; }

    /// 流起始扇区
    std::uint32_t startingSector() const { return startingSector_; }

    /// 流大小（字节）
    /// v3: 仅低 32 位有效；v4: 64 位有效
    std::uint64_t streamSize() const { return streamSize_; }

    bool isStream() const  { return objectType_ == ObjectType::Stream; }
    bool isStorage() const { return objectType_ == ObjectType::Storage; }
    bool isRoot() const    { return objectType_ == ObjectType::RootStorage; }
    bool hasLeftSibling() const  { return leftSiblingId_  != kNoStream; }
    bool hasRightSibling() const { return rightSiblingId_ != kNoStream; }
    bool hasChild() const        { return childId_        != kNoStream; }

private:
    std::u16string name_;
    std::uint16_t  nameLength_ = 0;
    ObjectType     objectType_ = ObjectType::Unknown;
    TreeNodeColor  color_ = TreeNodeColor::Black;
    std::uint32_t  leftSiblingId_ = kNoStream;
    std::uint32_t  rightSiblingId_ = kNoStream;
    std::uint32_t  childId_ = kNoStream;
    std::uint32_t  startingSector_ = 0;
    std::uint64_t  streamSize_ = 0;
};

/// 目录项表
class DirectoryEntryTable {
public:
    DirectoryEntryTable() = default;

    /// 添加一个扇区的目录数据（含多个 128 字节条目）
    void AddSector(const unsigned char* data, std::uint32_t sectorSize);

    /// 解析完成后获取所有目录项
    const std::vector<DirectoryEntry>& entries() const { return entries_; }

    /// 获取根目录项（通常是第 0 项）
    const DirectoryEntry* rootEntry() const;

    /// 按 ID 获取目录项
    const DirectoryEntry* entryById(std::uint32_t id) const;

    /// 在指定父目录下查找名为 name 的子项（不递归）
    /// @param parentId  父目录项 ID（通常是 Storage 或 Root）
    /// @param name      UTF-8 名称
    /// @return 找到的目录项 ID，未找到返回 kNoStream
    std::uint32_t findChild(std::uint32_t parentId, const std::string& name) const;

private:
    std::vector<DirectoryEntry> entries_;
};

} // namespace ole2
} // namespace office
} // namespace zq
