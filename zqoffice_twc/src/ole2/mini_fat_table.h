// =============================================================================
// src/ole2/mini_fat_table.h
//
// OLE2 MiniFAT 表
//
//   小于 miniStreamCutoff（默认 4096）的流走 MiniFAT
//   Mini 扇区大小固定 64 字节，存储在 Root Entry 的 MiniStream 中
// =============================================================================
#pragma once

#include <cstdint>
#include <vector>

#include "compound_file_header.h"

namespace zq {
namespace office {
namespace ole2 {

class MiniFATTable {
public:
    MiniFATTable() = default;

    void AddSector(const unsigned char* data, std::uint32_t sectorSize);

    std::uint32_t GetNextSector(std::uint32_t sector) const;

    std::vector<std::uint32_t> GetSectorChain(
        std::uint32_t startSector,
        std::size_t maxSectors = 100000) const;

    std::size_t entryCount() const { return entries_.size(); }

    static bool IsEndOfChain(std::uint32_t s) { return s == CompoundFileHeader::kEndOfChain; }

private:
    std::vector<std::uint32_t> entries_;
};

} // namespace ole2
} // namespace office
} // namespace zq
