// =============================================================================
// src/biff/biff_record_input_stream.h
//
// BIFF 记录输入流
//
//   - 在 OLE2 Stream 之上提供记录级读取
//   - 支持 Continue 记录拼接（BIFF 记录超过 8224 字节时拆分）
//   - 提供按字节/字/双字/浮点的数据读取
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

#include "biff_record_header.h"

namespace zq {
namespace office {
namespace biff {

/// BIFF 记录输入流
/// 在原始字节缓冲之上提供 BIFF 记录迭代与字段读取
class BIFFRecordInputStream {
public:
    BIFFRecordInputStream() = default;

    /// 从字节缓冲构造
    /// @param data  BIFF 流数据（通常是 OLE2 Workbook 流）
    /// @param size  字节数
    BIFFRecordInputStream(const unsigned char* data, std::size_t size);

    /// 是否还有未读记录
    bool HasNext() const;

    /// 读取下一条记录
    /// @return 记录头，失败返回 type=0 的空记录
    BIFFRecordHeader Next();

    /// 当前记录的数据指针
    const unsigned char* currentData() const { return currentData_; }

    /// 当前记录的数据长度
    std::uint16_t currentLength() const { return currentHeader_.length(); }

    /// 当前记录的类型
    std::uint16_t currentType() const { return currentHeader_.type(); }

    /// 跳过当前记录的数据部分
    void SkipCurrent();

    // -----------------------------------------------------------------------
    // 字段读取（在当前记录数据范围内）
    // -----------------------------------------------------------------------

    std::uint8_t  ReadByte();
    std::uint16_t ReadUShort();
    std::int16_t  ReadShort();
    std::uint32_t ReadUInt();
    std::int32_t  ReadInt();
    std::uint64_t ReadUInt64();
    float         ReadFloat();
    double        ReadDouble();

    /// 读取 N 字节到缓冲
    /// @return 实际读取的字节数
    std::size_t ReadBytes(unsigned char* out, std::size_t n);

    /// 读取剩余字节
    std::vector<unsigned char> ReadRemaining();

    /// 当前记录内偏移
    std::size_t position() const { return fieldPos_; }

    /// 重置字段读取位置到当前记录数据起始
    void ResetField();

    /// 原始数据指针
    const unsigned char* data() const { return data_; }

    /// 原始数据大小
    std::size_t size() const { return size_; }

    /// 当前记录在整个流中的字节偏移
    std::size_t recordOffset() const { return recordOffset_; }

private:
    const unsigned char* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t pos_ = 0;            // 流位置

    BIFFRecordHeader currentHeader_;
    const unsigned char* currentData_ = nullptr;
    std::size_t recordOffset_ = 0;   // 当前记录起始偏移
    std::size_t fieldPos_ = 0;       // 当前记录内字段偏移
};

} // namespace biff
} // namespace office
} // namespace zq
