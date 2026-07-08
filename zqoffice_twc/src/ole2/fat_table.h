// =============================================================================
// src/ole2/fat_table.h
//
// OLE2 FAT（File Allocation Table）扇区分配表
//
//   - Read  从扇区数据读取 FAT 条目
//   - GetNextSector  返回链中下一扇区号
//   - GetSectorChain  构建从起始扇区到 ENDOFCHAIN 的扇区链
//
// FAT 是一个 4 字节条目的数组，每个条目指向链中下一扇区
// 特殊值见 CompoundFileHeader::kXxx
// =============================================================================
#pragma once

#include <cstdint>
#include <vector>

#include "compound_file_header.h"

namespace zq {
namespace office {
namespace ole2 {

class FATTable {
public:
    FATTable() = default;

    /// 添加一个 FAT 扇区的数据（sectorSize 字节）
    /// 每个扇区含 sectorSize/4 个 4 字节条目
    void AddSector(const unsigned char* data, std::uint32_t sectorSize);

    /// 设置 FAT 总条目数（用于边界检查）
    void SetEntryCount(std::size_t count) { entries_.reserve(count); }

    /// 获取链中下一扇区
    /// @return 下一扇区号，ENDOFCHAIN 表示链尾
    std::uint32_t GetNextSector(std::uint32_t sector) const;

    /// 构建从 startSector 到 ENDOFCHAIN 的扇区链
    /// @param startSector  链起始扇区
    /// @param maxSectors  最大扇区数（防止环形链死循环），0 表示无限制
    /// @return 扇区号列表
    std::vector<std::uint32_t> GetSectorChain(
        std::uint32_t startSector,
        std::size_t maxSectors = 100000) const;

    /// FAT 条目总数
    std::size_t entryCount() const { return entries_.size(); }

    /// 直接访问 FAT 条目
    std::uint32_t operator[](std::size_t i) const { return entries_[i]; }

    /// 判断扇区号是否为特殊值
    static bool IsEndOfChain(std::uint32_t s) { return s == CompoundFileHeader::kEndOfChain; }
    static bool IsFreeSector(std::uint32_t s)  { return s == CompoundFileHeader::kFreeSector; }
    static bool IsFatSector(std::uint32_t s)   { return s == CompoundFileHeader::kFatSector; }
    static bool IsDifSector(std::uint32_t s)   { return s == CompoundFileHeader::kDifSector; }
    static bool IsSpecial(std::uint32_t s)     { return s >= 0xFFFFFFFCu; }

private:
    std::vector<std::uint32_t> entries_;
};

} // namespace ole2
} // namespace office
} // namespace zq
