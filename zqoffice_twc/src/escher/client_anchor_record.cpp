// =============================================================================
// src/escher/client_anchor_record.cpp
// =============================================================================
#include "client_anchor_record.h"
#include "../utils/little_endian.h"

namespace zq {
namespace office {
namespace escher {

bool ClientAnchorRecord::Read(const unsigned char* buf, std::size_t size) {
    if (!buf) return false;

    // 解析 Escher 头
    PptRecordHeader h;
    if (!h.Read(buf, size)) return false;
    if (h.recType() != kRecordTypeClientAnchor) return false;

    // 锚点数据在 Escher 头之后
    raw_ = buf + PptRecordHeader::kHeaderSize;
    rawSize_ = h.recLen();

    // 若为 PowerPoint 锚点（>=8 字节），解析 4×int16
    if (rawSize_ >= kPPTAnchorSize) {
        top_    = LittleEndian::getShort(const_cast<unsigned char*>(raw_ + 0), 0);
        left_   = LittleEndian::getShort(const_cast<unsigned char*>(raw_ + 2), 0);
        right_  = LittleEndian::getShort(const_cast<unsigned char*>(raw_ + 4), 0);
        bottom_ = LittleEndian::getShort(const_cast<unsigned char*>(raw_ + 6), 0);
    }
    return true;
}

} // namespace escher
} // namespace office
} // namespace zq
