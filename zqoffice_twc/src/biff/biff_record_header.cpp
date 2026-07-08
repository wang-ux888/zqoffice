// =============================================================================
// src/biff/biff_record_header.cpp
// =============================================================================
#include "biff_record_header.h"
#include "../utils/little_endian.h"

namespace zq {
namespace office {
namespace biff {

bool BIFFRecordHeader::Read(const unsigned char* buf, std::size_t size) {
    if (!buf || size < kHeaderSize) return false;
    type_   = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 0);
    length_ = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 2);
    return true;
}

} // namespace biff
} // namespace office
} // namespace zq
