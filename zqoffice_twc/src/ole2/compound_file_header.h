// =============================================================================
// src/ole2/compound_file_header.h
//
// OLE2 复合文档文件头解析（[MS-CFB] 2.2 节）
//
//   - Read  从字节缓冲读取文件头
//   - IsSectorBig  判断是否为 v4 大扇区（4096 字节）
//   - sectorSize / miniSectorSize / miniStreamCutoff 等访问器
//
// OLE2 文件头固定 512 字节，布局见 [MS-CFB] 2.2
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace zq {
namespace office {
namespace ole2 {

/// OLE2 复合文档文件头（512 字节）
class CompoundFileHeader {
public:
    static constexpr std::size_t kHeaderSize = 512;
    static constexpr std::uint64_t kMagic = 0xE11AB1A1E011CFD0ULL;

    // FAT 链特殊值（[MS-CFB] 2.2 节）
    static constexpr std::uint32_t kEndOfChain   = 0xFFFFFFFEu;
    static constexpr std::uint32_t kFatSector    = 0xFFFFFFFDu;
    static constexpr std::uint32_t kDifSector    = 0xFFFFFFFCu;
    static constexpr std::uint32_t kFreeSector   = 0xFFFFFFFFu;

    CompoundFileHeader() = default;

    /// 从 512 字节缓冲读取文件头
    /// @param buf  至少 kHeaderSize 字节的缓冲
    /// @return 解析成功返回 true
    bool Read(const unsigned char* buf, std::size_t size);

    /// 校验文件头是否合法
    bool IsValid() const;

    /// 是否为 v4 大扇区（SectorShift == 12，每扇区 4096 字节）
    /// v3 为 512 字节扇区
    bool IsSectorBig() const { return sectorShift_ >= 12; }

    // -----------------------------------------------------------------------
    // 访问器
    // -----------------------------------------------------------------------

    /// 扇区大小（字节）：v3=512，v4=4096
    std::uint32_t sectorSize() const { return 1u << sectorShift_; }

    /// Mini 扇区大小（字节）：通常 64
    std::uint32_t miniSectorSize() const { return 1u << miniSectorShift_; }

    /// MiniStream 截断阈值：小于此值的流走 MiniFAT
    std::uint32_t miniStreamCutoff() const { return miniStreamCutoff_; }

    std::uint16_t majorVersion() const { return majorVersion_; }
    std::uint16_t minorVersion() const { return minorVersion_; }
    std::uint16_t sectorShift() const { return sectorShift_; }
    std::uint16_t miniSectorShift() const { return miniSectorShift_; }

    /// FAT 链中的扇区总数
    std::uint32_t fatSectorCount() const { return fatSectorCount_; }

    /// 目录链首扇区
    std::uint32_t firstDirectorySector() const { return firstDirectorySector_; }

    /// MiniFAT 链首扇区
    std::uint32_t firstMiniFatSector() const { return firstMiniFatSector_; }

    /// MiniFAT 链扇区数
    std::uint32_t miniFatSectorCount() const { return miniFatSectorCount_; }

    /// DIFAT 链首扇区（Double-Indirect FAT）
    std::uint32_t firstDifatSector() const { return firstDifatSector_; }

    /// DIFAT 链扇区数
    std::uint32_t difatSectorCount() const { return difatSectorCount_; }

    /// 文件头内嵌的 DIFAT 数组（109 个条目）
    const std::uint32_t* difat() const { return difat_; }

    /// 获取 DIFAT 中第 i 个条目
    std::uint32_t difatAt(int i) const { return difat_[i]; }

private:
    std::uint64_t magic_ = 0;
    std::uint16_t minorVersion_ = 0;
    std::uint16_t majorVersion_ = 0;
    std::uint16_t sectorShift_ = 9;       // 默认 v3: 512 字节
    std::uint16_t miniSectorShift_ = 6;   // 默认 64 字节
    std::uint32_t fatSectorCount_ = 0;
    std::uint32_t firstDirectorySector_ = 0;
    std::uint32_t miniStreamCutoff_ = 4096;
    std::uint32_t firstMiniFatSector_ = kEndOfChain;
    std::uint32_t miniFatSectorCount_ = 0;
    std::uint32_t firstDifatSector_ = kEndOfChain;
    std::uint32_t difatSectorCount_ = 0;
    std::uint32_t difat_[109] = { 0 };    // 内嵌 DIFAT 数组
};

} // namespace ole2
} // namespace office
} // namespace zq
