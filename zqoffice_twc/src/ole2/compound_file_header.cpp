// =============================================================================
// src/ole2/compound_file_header.cpp
// =============================================================================
#include "compound_file_header.h"
#include "../utils/little_endian.h"

#include <cstring>

namespace zq {
namespace office {
namespace ole2 {

// 从字节缓冲读取文件头
bool CompoundFileHeader::Read(const unsigned char* buf, std::size_t size) {
    if (!buf || size < kHeaderSize) return false;

    // 魔数（8 字节小端）
    magic_ = 0;
    for (int i = 7; i >= 0; --i) {
        magic_ = (magic_ << 8) | buf[i];
    }
    if (magic_ != kMagic) return false;

    // CLSID（16 字节，offset 8-23）通常全 0，跳过

    minorVersion_        = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 24);
    majorVersion_        = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 26);
    // 字节序（offset 28，0xFFFE = little-endian）
    std::uint16_t byteOrder = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 28);
    if (byteOrder != 0xFFFE) return false;

    sectorShift_         = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 30);
    miniSectorShift_     = LittleEndian::getUShort(const_cast<unsigned char*>(buf), 32);

    // Reserved（6 字节，offset 34）跳过
    // TotalSectors（4 字节，offset 40，v4 必须为 0）跳过

    fatSectorCount_         = LittleEndian::getInt(const_cast<unsigned char*>(buf), 44);
    firstDirectorySector_   = LittleEndian::getInt(const_cast<unsigned char*>(buf), 48);
    // Transaction Signature（4 字节，offset 52）通常 0，跳过
    miniStreamCutoff_       = LittleEndian::getInt(const_cast<unsigned char*>(buf), 56);
    firstMiniFatSector_     = LittleEndian::getInt(const_cast<unsigned char*>(buf), 60);
    miniFatSectorCount_     = LittleEndian::getInt(const_cast<unsigned char*>(buf), 64);
    firstDifatSector_       = LittleEndian::getInt(const_cast<unsigned char*>(buf), 68);
    difatSectorCount_       = LittleEndian::getInt(const_cast<unsigned char*>(buf), 72);

    // DIFAT[109]（offset 76，109 × 4 = 436 字节）
    for (int i = 0; i < 109; ++i) {
        difat_[i] = LittleEndian::getInt(const_cast<unsigned char*>(buf), 76 + i * 4);
    }
    return true;
}

bool CompoundFileHeader::IsValid() const {
    if (magic_ != kMagic) return false;
    // v3: sectorShift=9, v4: sectorShift=12
    if (sectorShift_ != 9 && sectorShift_ != 12) return false;
    if (miniSectorShift_ != 6) return false;
    if (majorVersion_ != 3 && majorVersion_ != 4) return false;
    return true;
}

} // namespace ole2
} // namespace office
} // namespace zq
