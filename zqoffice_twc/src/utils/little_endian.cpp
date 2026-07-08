// =============================================================================
// src/utils/little_endian.cpp
// =============================================================================
#include "little_endian.h"

#include <cstring>

namespace zq {
namespace office {

unsigned char LittleEndian::getByte(unsigned char* b, int offset) {
    return b[offset];
}

short LittleEndian::getShort(unsigned char* b, int offset) {
    unsigned short v;
    std::memcpy(&v, b + offset, sizeof(unsigned short));
    return static_cast<short>(v);
}

unsigned short LittleEndian::getUShort(unsigned char* b, int offset) {
    unsigned short v;
    std::memcpy(&v, b + offset, sizeof(unsigned short));
    return v;
}

int LittleEndian::getInt(unsigned char* b, int offset) {
    int v;
    std::memcpy(&v, b + offset, sizeof(int));
    return v;
}

std::int64_t LittleEndian::getInt64(unsigned char* b, int offset) {
    std::int64_t v;
    std::memcpy(&v, b + offset, sizeof(std::int64_t));
    return v;
}

} // namespace office
} // namespace zq
