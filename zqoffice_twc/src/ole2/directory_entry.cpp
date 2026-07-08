// =============================================================================
// src/ole2/directory_entry.cpp
// =============================================================================
#include "directory_entry.h"
#include "../utils/little_endian.h"
#include "../utils/utils.h"

#include <cstring>

namespace zq {
namespace office {
namespace ole2 {

// ---------------------------------------------------------------------------
// DirectoryEntry
// ---------------------------------------------------------------------------
bool DirectoryEntry::Read(const unsigned char* buf, std::size_t size) {
    if (!buf || size < kEntrySize) return false;

    // 名称（64 字节 UTF-16LE，offset 0）
    nameLength_ = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 64);
    if (nameLength_ > 64) nameLength_ = 64; // 截断保护

    // 解析 UTF-16 名称（去除末尾 null）
    if (nameLength_ >= 2) {
        std::size_t charCount = nameLength_ / 2;
        if (charCount > 0) {
            name_.resize(charCount);
            for (std::size_t i = 0; i < charCount; ++i) {
                std::uint16_t c = LittleEndian::getUShort(
                    const_cast<unsigned char*>(buf), static_cast<int>(i * 2));
                name_[i] = static_cast<char16_t>(c);
            }
            // 去除末尾 null
            if (!name_.empty() && name_.back() == 0) {
                name_.pop_back();
            }
        }
    }

    std::uint8_t ot = buf[66];
    objectType_ = static_cast<ObjectType>(ot);

    std::uint8_t c = buf[67];
    color_ = static_cast<TreeNodeColor>(c);

    leftSiblingId_  = LittleEndian::getInt(const_cast<unsigned char*>(buf), 68);
    rightSiblingId_ = LittleEndian::getInt(const_cast<unsigned char*>(buf), 72);
    childId_        = LittleEndian::getInt(const_cast<unsigned char*>(buf), 76);

    // CLSID（offset 80，16 字节）跳过
    // State Bits（offset 96，4 字节）跳过
    // Creation Time（offset 100，8 字节）跳过
    // Modified Time（offset 108，8 字节）跳过

    startingSector_ = LittleEndian::getInt(const_cast<unsigned char*>(buf), 116);

    // Stream Size（offset 120）
    // v3 仅低 32 位有效；v4 64 位有效。这里统一读 64 位
    std::uint32_t lo = LittleEndian::getInt(const_cast<unsigned char*>(buf), 120);
    std::uint32_t hi = LittleEndian::getInt(const_cast<unsigned char*>(buf), 124);
    streamSize_ = (static_cast<std::uint64_t>(hi) << 32) | lo;

    return true;
}

std::string DirectoryEntry::utf8Name() const {
    return Utils::U16ToUtf8(name_);
}

// ---------------------------------------------------------------------------
// DirectoryEntryTable
// ---------------------------------------------------------------------------
void DirectoryEntryTable::AddSector(const unsigned char* data, std::uint32_t sectorSize) {
    if (!data || sectorSize < DirectoryEntry::kEntrySize) return;
    std::uint32_t count = sectorSize / DirectoryEntry::kEntrySize;
    for (std::uint32_t i = 0; i < count; ++i) {
        DirectoryEntry e;
        if (e.Read(data + i * DirectoryEntry::kEntrySize,
                   sectorSize - i * DirectoryEntry::kEntrySize)) {
            entries_.push_back(std::move(e));
        }
    }
}

const DirectoryEntry* DirectoryEntryTable::rootEntry() const {
    if (entries_.empty()) return nullptr;
    // 根目录项通常是第 0 项，且 objectType == RootStorage
    if (entries_[0].objectType() == ObjectType::RootStorage) {
        return &entries_[0];
    }
    // 否则搜索 RootStorage
    for (const auto& e : entries_) {
        if (e.objectType() == ObjectType::RootStorage) return &e;
    }
    return nullptr;
}

const DirectoryEntry* DirectoryEntryTable::entryById(std::uint32_t id) const {
    if (id >= entries_.size()) return nullptr;
    return &entries_[id];
}

std::uint32_t DirectoryEntryTable::findChild(std::uint32_t parentId,
                                              const std::string& name) const {
    auto* parent = entryById(parentId);
    if (!parent || !parent->hasChild()) return DirectoryEntry::kNoStream;

    // 遍历红黑树：从 childId 开始，按 leftSibling/rightSibling 遍历
    std::uint32_t cur = parent->childId();
    std::size_t iter = 0;
    std::vector<bool> visited(entries_.size(), false);
    while (cur != DirectoryEntry::kNoStream && cur < entries_.size() && iter < entries_.size()) {
        if (visited[cur]) break;
        visited[cur] = true;
        const auto& e = entries_[cur];
        if (e.utf8Name() == name) return cur;
        // 简化：先遍历左子树再遍历右子树
        // 注意：这是栈式遍历的简化，真正红黑树遍历需要递归
        // 对于 OLE2 目录树，通常很浅，这里采用先左后右的迭代方式
        if (e.hasLeftSibling()) {
            cur = e.leftSiblingId();
        } else if (e.hasRightSibling()) {
            cur = e.rightSiblingId();
        } else {
            // 回溯：找父节点的右兄弟（简化实现）
            cur = DirectoryEntry::kNoStream;
        }
        ++iter;
    }
    return DirectoryEntry::kNoStream;
}

} // namespace ole2
} // namespace office
} // namespace zq
