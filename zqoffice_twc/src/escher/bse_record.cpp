// =============================================================================
// src/escher/bse_record.cpp
// =============================================================================
#include "bse_record.h"
#include "../utils/little_endian.h"

#include <cstring>

namespace zq {
namespace office {
namespace escher {

bool BSERecord::Read(const unsigned char* buf, std::size_t size) {
    if (!buf) return false;

    // 先解析 Escher 头（8 字节）
    PptRecordHeader h;
    if (!h.Read(buf, size)) return false;
    if (h.recType() != kRecordTypeBSE) return false;

    // BSE 体在 Escher 头之后
    const unsigned char* body = buf + PptRecordHeader::kHeaderSize;
    std::size_t bodySize = h.recLen();
    if (bodySize < kBSEBodySize) return false;

    btWin32_ = body[0];
    btMacOS_ = body[1];
    std::memcpy(uid_, body + 2, 16);
    tag_      = LittleEndian::getUShort(const_cast<unsigned char*>(body + 18), 0);
    size_     = static_cast<std::uint32_t>(LittleEndian::getInt(
        const_cast<unsigned char*>(body + 20), 0));
    refcount_ = static_cast<std::uint32_t>(LittleEndian::getInt(
        const_cast<unsigned char*>(body + 24), 0));
    // unused1/unused2/unused3 在 body+28/+32/+36，跳过

    // 嵌入的 BLIP 记录在 BSE 体之后
    blipData_ = body + kBSEBodySize;
    blipSize_ = (bodySize >= kBSEBodySize) ? (bodySize - kBSEBodySize) : 0;
    return true;
}

} // namespace escher
} // namespace office
} // namespace zq
