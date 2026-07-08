// =============================================================================
// src/escher/blip_record.h
//
// BLIP（Binary Large Image/Photo）记录（[MS-ODRAW] 2.2.23-2.2.38）
//
//   BLIP 记录封装图像数据。Escher 头的 recInstance 字段指明图像格式。
//
//   元文件 BLIP（WMF/EMF/PICT）格式：
//     Header (8 字节 Escher)
//     MetafileHeader (26 字节)：
//       cb           (4 字节)  原始图像大小（缓存）
//       rcBounds     (8 字节)  边界矩形（top/left/right/bottom 各 2 字节）
//       ptSize       (8 字节)  原始尺寸（width/height 各 4 字节）
//       cbSave       (4 字节)  压缩后大小
//       fCompression (1 字节)  压缩类型（0=未压缩，1=压缩）
//       fFilter      (1 字节)  过滤类型（通常 0）
//     ImageData (cbSave 字节)  压缩的图像数据
//
//   位图 BLIP（JPEG/PNG/DIB/TIFF）格式：
//     Header (8 字节 Escher)
//     BitmapHeader (4 字节)：
//       cb (4 字节)  原始图像大小（缓存）
//     ImageData (cb 字节)  未压缩的图像数据
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>

#include "escher_record.h"

namespace zq {
namespace office {
namespace escher {

/// BLIP（图像）记录
class BlipRecord {
public:
    /// 元文件头大小（不含 Escher 头）
    static constexpr std::size_t kMetafileHeaderSize = 26;
    /// 位图头大小（不含 Escher 头）
    static constexpr std::size_t kBitmapHeaderSize = 4;

    BlipRecord() = default;

    /// 从缓冲解析 BLIP 记录
    /// @return 解析成功返回 true
    bool Read(const unsigned char* buf, std::size_t size);

    /// BLIP 类型（recInstance）
    std::uint16_t blipType() const { return blipType_; }

    /// BLIP 类型字符串名称
    const char* blipTypeName() const { return escher::blipTypeName(blipType_); }

    /// 是否为元文件（WMF/EMF/PICT）
    bool isMetafile() const {
        return blipType_ == kBlipTypeWMF
            || blipType_ == kBlipTypeEMF
            || blipType_ == kBlipTypePICT;
    }

    /// 是否为位图（JPEG/PNG/DIB/TIFF）
    bool isBitmap() const {
        return blipType_ == kBlipTypeJPEG
            || blipType_ == kBlipTypePNG
            || blipType_ == kBlipTypeDIB
            || blipType_ == kBlipTypeTIFF;
    }

    /// 原始图像大小（cb，未压缩时）
    std::uint32_t cb() const { return cb_; }

    /// 元文件：压缩后大小；位图：未使用（与 cb 相同）
    std::uint32_t cbSave() const { return cbSave_; }

    /// 元文件：压缩类型（0=未压缩，1=压缩）；位图：未使用
    std::uint8_t fCompression() const { return fCompression_; }

    /// 边界矩形 top（仅元文件有效）
    std::int16_t boundsTop() const { return boundsTop_; }
    /// 边界矩形 left
    std::int16_t boundsLeft() const { return boundsLeft_; }
    /// 边界矩形 right
    std::int16_t boundsRight() const { return boundsRight_; }
    /// 边界矩形 bottom
    std::int16_t boundsBottom() const { return boundsBottom_; }

    /// 原始宽度（仅元文件有效）
    std::int32_t ptWidth() const { return ptWidth_; }
    /// 原始高度（仅元文件有效）
    std::int32_t ptHeight() const { return ptHeight_; }

    /// 图像数据指针（已解压前的原始字节）
    /// 位图：直接可用的图像字节流；元文件：可能压缩的字节流
    const unsigned char* imageData() const { return imageData_; }

    /// 图像数据字节数
    std::size_t imageSize() const { return imageSize_; }

private:
    std::uint16_t blipType_ = 0;

    // 共有字段
    std::uint32_t cb_ = 0;

    // 元文件特有字段
    std::uint32_t cbSave_ = 0;
    std::uint8_t  fCompression_ = 0;
    std::int16_t  boundsTop_ = 0;
    std::int16_t  boundsLeft_ = 0;
    std::int16_t  boundsRight_ = 0;
    std::int16_t  boundsBottom_ = 0;
    std::int32_t  ptWidth_ = 0;
    std::int32_t  ptHeight_ = 0;

    const unsigned char* imageData_ = nullptr;
    std::size_t imageSize_ = 0;
};

} // namespace escher
} // namespace office
} // namespace zq
