// =============================================================================
// src/escher/blip_record.cpp
// =============================================================================
#include "blip_record.h"
#include "../utils/little_endian.h"

namespace zq {
namespace office {
namespace escher {

bool BlipRecord::Read(const unsigned char* buf, std::size_t size) {
    if (!buf) return false;

    // 解析 Escher 头
    PptRecordHeader h;
    if (!h.Read(buf, size)) return false;
    // 通过 recType 范围检查是否为 BLIP 记录
    if (h.recType() < kRecordTypeBlipFirst || h.recType() > kRecordTypeBlipLast)
        return false;

    blipType_ = h.recInstance();
    const unsigned char* body = buf + PptRecordHeader::kHeaderSize;
    std::size_t bodySize = h.recLen();

    if (isMetafile()) {
        // MetafileHeader (26 字节)
        if (bodySize < kMetafileHeaderSize) return false;

        cb_           = static_cast<std::uint32_t>(LittleEndian::getInt(
            const_cast<unsigned char*>(body + 0), 0));
        boundsTop_    = LittleEndian::getShort(const_cast<unsigned char*>(body + 4), 0);
        boundsLeft_   = LittleEndian::getShort(const_cast<unsigned char*>(body + 6), 0);
        boundsRight_  = LittleEndian::getShort(const_cast<unsigned char*>(body + 8), 0);
        boundsBottom_ = LittleEndian::getShort(const_cast<unsigned char*>(body + 10), 0);
        ptWidth_      = LittleEndian::getInt(const_cast<unsigned char*>(body + 12), 0);
        ptHeight_     = LittleEndian::getInt(const_cast<unsigned char*>(body + 16), 0);
        cbSave_       = static_cast<std::uint32_t>(LittleEndian::getInt(
            const_cast<unsigned char*>(body + 20), 0));
        fCompression_ = body[24];
        // body[25] = fFilter，跳过

        imageData_ = body + kMetafileHeaderSize;
        imageSize_ = (bodySize >= kMetafileHeaderSize) ? (bodySize - kMetafileHeaderSize) : 0;
        // 若 cbSave_ 有效且小于等于 imageSize_，使用 cbSave_
        if (cbSave_ > 0 && cbSave_ <= imageSize_) {
            imageSize_ = cbSave_;
        }
    } else if (isBitmap()) {
        // BitmapHeader (4 字节)
        if (bodySize < kBitmapHeaderSize) return false;

        cb_ = static_cast<std::uint32_t>(LittleEndian::getInt(
            const_cast<unsigned char*>(body + 0), 0));
        cbSave_ = cb_;  // 位图无压缩，cbSave 与 cb 一致
        fCompression_ = 0;

        imageData_ = body + kBitmapHeaderSize;
        imageSize_ = (bodySize >= kBitmapHeaderSize) ? (bodySize - kBitmapHeaderSize) : 0;
        // 若 cb_ 有效且小于等于 imageSize_，使用 cb_
        if (cb_ > 0 && cb_ <= imageSize_) {
            imageSize_ = cb_;
        }
    } else {
        // 未知 BLIP 类型，作为原始数据保留
        imageData_ = body;
        imageSize_ = bodySize;
    }
    return true;
}

} // namespace escher
} // namespace office
} // namespace zq
