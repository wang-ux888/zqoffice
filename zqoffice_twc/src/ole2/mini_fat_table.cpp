// =============================================================================
// src/ole2/mini_fat_table.cpp
// =============================================================================
#include "mini_fat_table.h"
#include "../utils/little_endian.h"

namespace zq {
namespace office {
namespace ole2 {

void MiniFATTable::AddSector(const unsigned char* data, std::uint32_t sectorSize) {
    if (!data || sectorSize < 4) return;
    std::uint32_t count = sectorSize / 4;
    for (std::uint32_t i = 0; i < count; ++i) {
        entries_.push_back(
            LittleEndian::getInt(const_cast<unsigned char*>(data), i * 4));
    }
}

std::uint32_t MiniFATTable::GetNextSector(std::uint32_t sector) const {
    if (sector >= entries_.size()) return CompoundFileHeader::kEndOfChain;
    return entries_[sector];
}

std::vector<std::uint32_t> MiniFATTable::GetSectorChain(
    std::uint32_t startSector, std::size_t maxSectors) const {
    std::vector<std::uint32_t> chain;
    if (IsEndOfChain(startSector) || startSector == CompoundFileHeader::kFreeSector) {
        return chain;
    }
    std::uint32_t cur = startSector;
    std::size_t iter = 0;
    std::vector<bool> visited(entries_.size(), false);
    while (!IsEndOfChain(cur) && cur < entries_.size() && iter < maxSectors) {
        if (visited[cur]) break;
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
