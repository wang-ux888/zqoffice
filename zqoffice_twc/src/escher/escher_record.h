// =============================================================================
// src/escher/escher_record.h
//
// Escher 记录基础类（[MS-ODRAW] / [MS-OFFCRYPTO] OfficeArt 结构）
//
//   Escher 记录格式与 PPT 记录完全相同（8 字节头）：
//     Offset 0: RecVer (4 bits) + RecInstance (12 bits) → 2 字节
//     Offset 2: RecType (2 字节)
//     Offset 4: RecLen (4 字节)
//     Offset 8: Record Data (N 字节)
//
//   RecVer=0xF 表示容器记录（包含子记录），其他值表示原子记录
//
// 常用记录类型（[MS-ODRAW] 2.4 节）：
//   0xF000  DggContainer       OfficeArtDrawingContainer
//   0xF001  BStoreContainer    OfficeArtBStoreContainer（含 BSE 子记录）
//   0xF002  DGContainer        OfficeArtDGContainer
//   0xF003  SpgrContainer      OfficeArtSpgrContainer
//   0xF004  SpContainer        OfficeArtSpContainer
//   0xF005  OPT                OfficeArtFOPT（形状属性表）
//   0xF006  TertiaryFOPT       OfficeArtTertiaryFOPT
//   0xF007  BSE                OfficeArtBStoreContainerEntry（Blip Store Entry）
//   0xF008  ChildAnchor        OfficeArtChildAnchor
//   0xF009  ClientAnchor       OfficeArtClientAnchor
//   0xF00A  ClientData         OfficeArtClientData
//   0xF00B  ColorMRU           OfficeArtColorMRU
//   0xF00D  Collection         OfficeArtBlipCollection9
//   0xF011  SecondaryFOPT      OfficeArtSecondaryFOPT
//   0xF11E  SplitMenuColor     OfficeArtSplitMenuColorRecord
//   0xF018-0xF117  BLIP        BLIP 记录（图像数据，recInstance 指明图像格式）
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include "../biff/ppt_record_header.h"

namespace zq {
namespace office {
namespace escher {

// PptRecordHeader 定义在 biff 命名空间，此处引入以便简洁引用
using zq::office::biff::PptRecordHeader;

// ---------------------------------------------------------------------------
// Escher 记录类型常量（[MS-ODRAW] 2.4 节）
// ---------------------------------------------------------------------------
constexpr std::uint16_t kRecordTypeDggContainer    = 0xF000;
constexpr std::uint16_t kRecordTypeBStoreContainer = 0xF001;
constexpr std::uint16_t kRecordTypeDGContainer     = 0xF002;
constexpr std::uint16_t kRecordTypeSpgrContainer   = 0xF003;
constexpr std::uint16_t kRecordTypeSpContainer     = 0xF004;
constexpr std::uint16_t kRecordTypeOPT             = 0xF005;
constexpr std::uint16_t kRecordTypeTertiaryFOPT    = 0xF006;
constexpr std::uint16_t kRecordTypeBSE             = 0xF007;
constexpr std::uint16_t kRecordTypeChildAnchor     = 0xF008;
constexpr std::uint16_t kRecordTypeClientAnchor    = 0xF009;
constexpr std::uint16_t kRecordTypeClientData      = 0xF00A;
constexpr std::uint16_t kRecordTypeColorMRU        = 0xF00B;
constexpr std::uint16_t kRecordTypeCollection      = 0xF00D;
constexpr std::uint16_t kRecordTypeFRITContainer   = 0xF010;
constexpr std::uint16_t kRecordTypeSecondaryFOPT   = 0xF011;
constexpr std::uint16_t kRecordTypeSplitMenuColor  = 0xF11E;
constexpr std::uint16_t kRecordTypeBlipFirst       = 0xF018;
constexpr std::uint16_t kRecordTypeBlipLast        = 0xF117;

// ---------------------------------------------------------------------------
// BLIP 图像格式（recInstance of BLIP record，[MS-ODRAW] 2.4.63-2.4.70）
// ---------------------------------------------------------------------------
constexpr std::uint16_t kBlipTypeError   = 0x00;
constexpr std::uint16_t kBlipTypeUnknown = 0x01;
constexpr std::uint16_t kBlipTypeWMF     = 0x02;
constexpr std::uint16_t kBlipTypeEMF     = 0x03;
constexpr std::uint16_t kBlipTypePICT    = 0x04;
constexpr std::uint16_t kBlipTypeJPEG    = 0x05;
constexpr std::uint16_t kBlipTypePNG     = 0x06;
constexpr std::uint16_t kBlipTypeDIB     = 0x07;
constexpr std::uint16_t kBlipTypeTIFF    = 0x08;

/// 返回 BLIP 类型的字符串名称（用于日志/调试）
const char* blipTypeName(std::uint16_t blipType);

/// 返回 Escher 记录类型的字符串名称
const char* recordTypeName(std::uint16_t recType);

/// Escher 记录：在 PptRecordHeader 之上提供记录数据访问
///
///   - 不持有数据缓冲，仅记录指针与长度（视图语义）
///   - 容器记录的子记录通过 RecordEnumerator<PptRecordHeader> 迭代
class EscherRecord {
public:
    EscherRecord() = default;

    /// 从缓冲解析一条 Escher 记录
    /// @param buf  数据缓冲（至少 kHeaderSize 字节）
    /// @param size 缓冲字节数
    /// @return 解析成功返回 true
    bool Read(const unsigned char* buf, std::size_t size);

    /// 头部
    const PptRecordHeader& header() const { return header_; }

    /// 记录类型（recType）
    std::uint16_t type() const { return header_.recType(); }

    /// 数据长度（recLen）
    std::uint32_t length() const { return header_.recLen(); }

    /// 记录实例（recInstance，BLIP 记录用此字段区分图像格式）
    std::uint16_t instance() const { return header_.recInstance(); }

    /// 记录版本（recVer，0xF=容器）
    std::uint8_t version() const { return header_.recVer(); }

    /// 是否为容器记录
    bool isContainer() const { return header_.isContainer(); }

    /// 是否为原子记录
    bool isAtom() const { return header_.isAtom(); }

    /// 记录数据指针（不含头）
    const unsigned char* data() const { return data_; }

    /// 记录总大小（头 + 数据）
    std::size_t totalSize() const {
        return PptRecordHeader::kHeaderSize + header_.recLen();
    }

    /// 是否为 BLIP 记录（图像数据）
    bool isBlip() const {
        return type() >= kRecordTypeBlipFirst && type() <= kRecordTypeBlipLast;
    }

    /// 是否为 BSE 记录（Blip Store Entry）
    bool isBSE() const { return type() == kRecordTypeBSE; }

    /// 是否为 ClientAnchor 记录
    bool isClientAnchor() const { return type() == kRecordTypeClientAnchor; }

    /// 是否为 OPT 记录（形状属性表）
    bool isOPT() const { return type() == kRecordTypeOPT; }

private:
    PptRecordHeader header_{};
    const unsigned char* data_ = nullptr;
};

} // namespace escher
} // namespace office
} // namespace zq
