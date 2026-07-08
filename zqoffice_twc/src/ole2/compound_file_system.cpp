// =============================================================================
// src/ole2/compound_file_system.cpp
// =============================================================================
#include "compound_file_system.h"
#include "../utils/little_endian.h"
#include "../utils/utils.h"

#include <algorithm>
#include <sstream>
#include <cstring>

namespace zq {
namespace office {
namespace ole2 {

// ---------------------------------------------------------------------------
// 加载入口
// ---------------------------------------------------------------------------
bool CompoundFileSystem::Load(const std::vector<unsigned char>& data) {
    loaded_ = false;
    error_.clear();
    fileData_ = data;

    if (fileData_.size() < CompoundFileHeader::kHeaderSize) {
        error_ = "file too small (< 512 bytes)";
        return false;
    }

    // 1. 解析文件头
    if (!header_.Read(fileData_.data(), fileData_.size())) {
        error_ = "invalid header magic";
        return false;
    }
    if (!header_.IsValid()) {
        error_ = "header validation failed";
        return false;
    }

    // 2. 读取 DIFAT，收集所有 FAT 扇区号
    if (!ReadDifat()) {
        error_ = "failed to read DIFAT";
        return false;
    }

    // 3. 构建 FAT 表
    if (!BuildFatTable()) {
        error_ = "failed to build FAT";
        return false;
    }

    // 4. 构建 Directory
    if (!BuildDirectory()) {
        error_ = "failed to build directory";
        return false;
    }

    // 5. 构建 MiniStream（Root Entry 的数据流）
    if (!BuildMiniStream()) {
        // MiniStream 可能不存在（极简文件），不视为致命错误
    }

    // 6. 构建 MiniFAT
    if (!BuildMiniFatTable()) {
        // MiniFAT 可能不存在，不视为致命错误
    }

    loaded_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// DIFAT
// ---------------------------------------------------------------------------
bool CompoundFileSystem::ReadDifat() {
    difatSectors_.clear();

    // 文件头内嵌的 109 个 DIFAT 条目
    for (int i = 0; i < 109; ++i) {
        std::uint32_t s = header_.difatAt(i);
        if (s == CompoundFileHeader::kFreeSector) continue;
        if (s == CompoundFileHeader::kEndOfChain) break;
        difatSectors_.push_back(s);
    }

    // DIFAT 链（如果文件大于 109 个 FAT 扇区）
    std::uint32_t cur = header_.firstDifatSector();
    std::uint32_t sectorSize = header_.sectorSize();
    std::size_t iter = 0;
    std::vector<bool> visited;
    visited.assign(fileData_.size() / sectorSize, false);

    while (cur != CompoundFileHeader::kEndOfChain &&
           cur != CompoundFileHeader::kFreeSector &&
           iter < 100000) {
        std::size_t off = SectorOffset(cur);
        if (off + sectorSize > fileData_.size()) break;
        if (cur < visited.size() && visited[cur]) break;
        if (cur < visited.size()) visited[cur] = true;

        // DIFAT 扇区：前 (sectorSize - 4) 字节是 FAT 扇区号，最后 4 字节是下一 DIFAT 扇区
        std::uint32_t entries = (sectorSize - 4) / 4;
        for (std::uint32_t i = 0; i < entries; ++i) {
            std::uint32_t s = LittleEndian::getInt(
                const_cast<unsigned char*>(fileData_.data() + off), i * 4);
            if (s == CompoundFileHeader::kFreeSector) continue;
            if (s == CompoundFileHeader::kEndOfChain) break;
            difatSectors_.push_back(s);
        }
        cur = LittleEndian::getInt(
            const_cast<unsigned char*>(fileData_.data() + off), sectorSize - 4);
        ++iter;
    }
    return true;
}

// ---------------------------------------------------------------------------
// FAT
// ---------------------------------------------------------------------------
bool CompoundFileSystem::BuildFatTable() {
    std::uint32_t sectorSize = header_.sectorSize();
    fat_.SetEntryCount(difatSectors_.size() * (sectorSize / 4));
    for (std::uint32_t s : difatSectors_) {
        std::size_t off = SectorOffset(s);
        if (off + sectorSize > fileData_.size()) {
            error_ = "FAT sector out of range";
            return false;
        }
        fat_.AddSector(fileData_.data() + off, sectorSize);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Directory
// ---------------------------------------------------------------------------
bool CompoundFileSystem::BuildDirectory() {
    std::uint32_t sectorSize = header_.sectorSize();
    auto chain = fat_.GetSectorChain(header_.firstDirectorySector());
    for (std::uint32_t s : chain) {
        std::size_t off = SectorOffset(s);
        if (off + sectorSize > fileData_.size()) continue;
        directory_.AddSector(fileData_.data() + off, sectorSize);
    }
    return !directory_.entries().empty();
}

// ---------------------------------------------------------------------------
// MiniStream（Root Entry 的数据流）
// ---------------------------------------------------------------------------
bool CompoundFileSystem::BuildMiniStream() {
    const DirectoryEntry* root = directory_.rootEntry();
    if (!root) return false;
    if (root->streamSize() == 0) return false;

    auto chain = fat_.GetSectorChain(root->startingSector());
    miniStream_ = ReadSectorChain(chain, root->streamSize());
    return !miniStream_.empty();
}

// ---------------------------------------------------------------------------
// MiniFAT
// ---------------------------------------------------------------------------
bool CompoundFileSystem::BuildMiniFatTable() {
    if (header_.firstMiniFatSector() == CompoundFileHeader::kEndOfChain) {
        return true; // 无 MiniFAT
    }
    std::uint32_t sectorSize = header_.sectorSize();
    auto chain = fat_.GetSectorChain(header_.firstMiniFatSector());
    for (std::uint32_t s : chain) {
        std::size_t off = SectorOffset(s);
        if (off + sectorSize > fileData_.size()) continue;
        miniFat_.AddSector(fileData_.data() + off, sectorSize);
    }
    return true;
}

// ---------------------------------------------------------------------------
// 扇区读取
// ---------------------------------------------------------------------------
std::vector<unsigned char> CompoundFileSystem::ReadSectorChain(
    const std::vector<std::uint32_t>& chain, std::uint64_t maxSize) {
    std::vector<unsigned char> result;
    if (chain.empty()) return result;
    std::uint32_t sectorSize = header_.sectorSize();
    result.reserve(chain.size() * sectorSize);
    std::uint64_t total = 0;
    for (std::uint32_t s : chain) {
        if (total >= maxSize) break;
        std::size_t off = SectorOffset(s);
        if (off + sectorSize > fileData_.size()) break;
        std::uint32_t copySize = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(sectorSize, maxSize - total));
        result.insert(result.end(),
                      fileData_.data() + off,
                      fileData_.data() + off + copySize);
        total += copySize;
    }
    // 截断到实际流大小
    if (result.size() > maxSize) result.resize(maxSize);
    return result;
}

std::vector<unsigned char> CompoundFileSystem::ReadMiniSectorChain(
    const std::vector<std::uint32_t>& chain, std::uint64_t maxSize) {
    std::vector<unsigned char> result;
    if (chain.empty() || miniStream_.empty()) return result;
    std::uint32_t miniSectorSize = header_.miniSectorSize();
    result.reserve(chain.size() * miniSectorSize);
    std::uint64_t total = 0;
    for (std::uint32_t s : chain) {
        if (total >= maxSize) break;
        std::size_t off = s * miniSectorSize;
        if (off + miniSectorSize > miniStream_.size()) break;
        std::uint32_t copySize = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(miniSectorSize, maxSize - total));
        result.insert(result.end(),
                      miniStream_.data() + off,
                      miniStream_.data() + off + copySize);
        total += copySize;
    }
    if (result.size() > maxSize) result.resize(maxSize);
    return result;
}

// ---------------------------------------------------------------------------
// 流打开
// ---------------------------------------------------------------------------
Stream CompoundFileSystem::OpenStream(const std::string& name) {
    Stream s;
    s.name = name;
    if (!loaded_) return s;

    std::uint32_t entryId = FindEntryByPath(name);
    if (entryId == DirectoryEntry::kNoStream) return s;
    return OpenStreamById(entryId);
}

Stream CompoundFileSystem::OpenStreamById(std::uint32_t entryId) {
    Stream s;
    const DirectoryEntry* e = directory_.entryById(entryId);
    if (!e || !e->isStream()) return s;
    s.name = e->utf8Name();
    s.size = e->streamSize();

    if (e->streamSize() < header_.miniStreamCutoff()) {
        // 走 MiniFAT
        auto chain = miniFat_.GetSectorChain(e->startingSector());
        s.data = ReadMiniSectorChain(chain, e->streamSize());
    } else {
        // 走 FAT
        auto chain = fat_.GetSectorChain(e->startingSector());
        s.data = ReadSectorChain(chain, e->streamSize());
    }
    return s;
}

// ---------------------------------------------------------------------------
// 列举
// ---------------------------------------------------------------------------
std::vector<std::string> CompoundFileSystem::ListRootEntries() const {
    const DirectoryEntry* root = directory_.rootEntry();
    if (!root) return {};
    return ListChildren(static_cast<std::uint32_t>(
        root - directory_.entries().data()));
}

std::vector<std::string> CompoundFileSystem::ListChildren(std::uint32_t parentId) const {
    std::vector<std::string> result;
    const DirectoryEntry* parent = directory_.entryById(parentId);
    if (!parent || !parent->hasChild()) return result;

    // 中序遍历红黑树
    std::uint32_t cur = parent->childId();
    std::vector<bool> visited(directory_.entries().size(), false);
    std::size_t iter = 0;
    // 使用栈模拟中序遍历
    std::vector<std::uint32_t> stack;
    while ((cur != DirectoryEntry::kNoStream || !stack.empty()) &&
           iter < directory_.entries().size()) {
        while (cur != DirectoryEntry::kNoStream && cur < directory_.entries().size() &&
               !visited[cur]) {
            stack.push_back(cur);
            visited[cur] = true;
            const auto& e = directory_.entries()[cur];
            cur = e.hasLeftSibling() ? e.leftSiblingId() : DirectoryEntry::kNoStream;
        }
        if (stack.empty()) break;
        cur = stack.back();
        stack.pop_back();
        const auto& e = directory_.entries()[cur];
        result.push_back(e.utf8Name());
        cur = e.hasRightSibling() ? e.rightSiblingId() : DirectoryEntry::kNoStream;
        ++iter;
    }
    return result;
}

// ---------------------------------------------------------------------------
// 路径查找
// ---------------------------------------------------------------------------
std::uint32_t CompoundFileSystem::FindEntryByPath(const std::string& path) const {
    if (path.empty()) return DirectoryEntry::kNoStream;

    // 拆分路径
    std::vector<std::string> segments;
    std::string seg;
    std::istringstream iss(path);
    while (std::getline(iss, seg, '/')) {
        if (!seg.empty()) segments.push_back(seg);
    }
    if (segments.empty()) return DirectoryEntry::kNoStream;

    // 从根目录开始
    const DirectoryEntry* root = directory_.rootEntry();
    if (!root) return DirectoryEntry::kNoStream;
    std::uint32_t rootId = static_cast<std::uint32_t>(
        root - directory_.entries().data());

    // 第一段可能是 "Root Entry"，跳过
    std::size_t startIdx = 0;
    if (segments[0] == root->utf8Name()) {
        startIdx = 1;
        if (startIdx >= segments.size()) return rootId;
    }

    return FindEntryRecursive(rootId, segments, startIdx);
}

std::uint32_t CompoundFileSystem::FindEntryRecursive(
    std::uint32_t parentId,
    const std::vector<std::string>& segments,
    std::size_t segIdx) const {
    if (segIdx >= segments.size()) return parentId;
    std::uint32_t childId = FindInStorage(parentId, segments[segIdx]);
    if (childId == DirectoryEntry::kNoStream) return DirectoryEntry::kNoStream;
    if (segIdx + 1 >= segments.size()) return childId;
    return FindEntryRecursive(childId, segments, segIdx + 1);
}

std::uint32_t CompoundFileSystem::FindInStorage(std::uint32_t storageId,
                                                  const std::string& name) const {
    const DirectoryEntry* storage = directory_.entryById(storageId);
    if (!storage || !storage->hasChild()) return DirectoryEntry::kNoStream;

    // 将 name 转 UTF-16
    std::u16string target = Utils::Utf8ToU16(name);
    return RbTreeFind(storage->childId(), target);
}

std::uint32_t CompoundFileSystem::RbTreeFind(std::uint32_t rootId,
                                              const std::u16string& targetName) const {
    // 红黑树查找：按名称比较（不区分大小写）
    std::uint32_t cur = rootId;
    std::vector<bool> visited(directory_.entries().size(), false);
    std::size_t iter = 0;
    while (cur != DirectoryEntry::kNoStream && cur < directory_.entries().size() &&
           iter < directory_.entries().size()) {
        if (visited[cur]) break;
        visited[cur] = true;
        const auto& e = directory_.entries()[cur];

        // 名称比较（大小写不敏感，UTF-16）
        int cmp = CompareNameIgnoreCase(e.name(), targetName);
        if (cmp == 0) return cur;
        if (cmp < 0) {
            cur = e.hasRightSibling() ? e.rightSiblingId() : DirectoryEntry::kNoStream;
        } else {
            cur = e.hasLeftSibling() ? e.leftSiblingId() : DirectoryEntry::kNoStream;
        }
        ++iter;
    }
    return DirectoryEntry::kNoStream;
}

// 静态工具：UTF-16 名称大小写不敏感比较
// 返回 <0 表示 a < b，0 表示相等，>0 表示 a > b
int CompoundFileSystem::CompareNameIgnoreCase(const std::u16string& a,
                                               const std::u16string& b) {
    std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        char16_t ca = a[i];
        char16_t cb = b[i];
        // 简单 ASCII 大小写转换（不处理 Unicode 大小写折叠，需要 ICU）
        if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
        if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
        if (ca != cb) return ca < cb ? -1 : 1;
    }
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return 1;
    return 0;
}

} // namespace ole2
} // namespace office
} // namespace zq
