// =============================================================================
// src/ole2/fat_table.cpp
// =============================================================================
#include "fat_table.h"
#include "../utils/little_endian.h"

#include <algorithm>

namespace zq {
namespace office {
namespace ole2 {

void FATTable::AddSector(const unsigned char* data, std::uint32_t sectorSize) {
    if (!data || sectorSize < 4) return;
    std::uint32_t count = sectorSize / 4;
    for (std::uint32_t i = 0; i < count; ++i) {
        entries_.push_back(
            LittleEndian::getInt(const_cast<unsigned char*>(data), i * 4));
    }
}

std::uint32_t FATTable::GetNextSector(std::uint32_t sector) const {
    if (sector >= entries_.size()) return CompoundFileHeader::kEndOfChain;
    return entries_[sector];
}

std::vector<std::uint32_t> FATTable::GetSectorChain(
    std::uint32_t startSector, std::size_t maxSectors) const {
    std::vector<std::uint32_t> chain;
    if (IsSpecial(startSector)) return chain;

    std::uint32_t cur = startSector;
    std::size_t iter = 0;
    // 防止环形链：使用集合检测重复
    std::vector<bool> visited(entries_.size(), false);

    while (!IsEndOfChain(cur) && !IsFreeSector(cur) &&
           iter < maxSectors && cur < entries_.size()) {
        if (visited[cur]) break; // 环形链保护
        visited[cur] = true;
        chain.push_back(cur);
        cur = entries_[cur];
        ++iter;
    }
    return chain;
}

} // namespace ole2
} // namespace office
} // namespace zq
