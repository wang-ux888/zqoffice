// =============================================================================
// src/biff/ppt_record_header.cpp
// =============================================================================
#include "ppt_record_header.h"
#include "../utils/little_endian.h"

namespace zq {
namespace office {
namespace biff {

bool PptRecordHeader::Read(const unsigned char* buf, std::size_t size) {
    if (!buf || size < kHeaderSize) return false;

    // Offset 0: RecVer (4 bits) + RecInstance (12 bits)
    std::uint16_t verInstance = LittleEndian::getUShort(
        const_cast<unsigned char*>(buf), 0);
    recVer_       = static_cast<std::uint8_t>(verInstance & 0x000F);
    recInstance_  = static_cast<std::uint16_t>((verInstance >> 4) & 0x0FFF);

    // Offset 2: RecType
    recType_ = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 2);

    // Offset 4: RecLen
    recLen_ = LittleEndian::getInt(const_cast<unsigned char*>(buf), 4);

    return true;
}

} // namespace biff
} // namespace office
} // namespace zq
