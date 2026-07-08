// =============================================================================
// src/biff/biff_record_input_stream.cpp
// =============================================================================
#include "biff_record_input_stream.h"
#include "../utils/little_endian.h"

#include <cstring>

namespace zq {
namespace office {
namespace biff {

BIFFRecordInputStream::BIFFRecordInputStream(const unsigned char* data, std::size_t size)
    : data_(data), size_(size) {}

bool BIFFRecordInputStream::HasNext() const {
    if (!data_ || pos_ >= size_) return false;
    return pos_ + BIFFRecordHeader::kHeaderSize <= size_;
}

BIFFRecordHeader BIFFRecordInputStream::Next() {
    BIFFRecordHeader h;
    if (!HasNext()) return h;
    if (!h.Read(data_ + pos_, size_ - pos_)) return h;

    currentHeader_ = h;
    currentData_ = data_ + pos_ + BIFFRecordHeader::kHeaderSize;
    recordOffset_ = pos_;
    fieldPos_ = 0;

    // 推进到下一条记录起点（不跳过数据，数据通过 ReadXxx 读取）
    pos_ += BIFFRecordHeader::kHeaderSize + h.length();
    return h;
}

void BIFFRecordInputStream::SkipCurrent() {
    // Next() 已经把 pos_ 推进到下一条记录，这里只重置字段位置
    fieldPos_ = currentHeader_.length();
}

// ---------------------------------------------------------------------------
// 字段读取
// ---------------------------------------------------------------------------
std::uint8_t BIFFRecordInputStream::ReadByte() {
    if (!currentData_ || fieldPos_ >= currentHeader_.length()) return 0;
    return currentData_[fieldPos_++];
}

std::uint16_t BIFFRecordInputStream::ReadUShort() {
    if (!currentData_ || fieldPos_ + 2 > currentHeader_.length()) return 0;
    std::uint16_t v = LittleEndian::getUShort(
        const_cast<unsigned char*>(currentData_),
        static_cast<int>(fieldPos_));
    fieldPos_ += 2;
    return v;
}

std::int16_t BIFFRecordInputStream::ReadShort() {
    return static_cast<std::int16_t>(ReadUShort());
}

std::uint32_t BIFFRecordInputStream::ReadUInt() {
    if (!currentData_ || fieldPos_ + 4 > currentHeader_.length()) return 0;
    std::uint32_t v = static_cast<std::uint32_t>(LittleEndian::getInt(
        const_cast<unsigned char*>(currentData_),
        static_cast<int>(fieldPos_)));
    fieldPos_ += 4;
    return v;
}

std::int32_t BIFFRecordInputStream::ReadInt() {
    return static_cast<std::int32_t>(ReadUInt());
}

std::uint64_t BIFFRecordInputStream::ReadUInt64() {
    if (!currentData_ || fieldPos_ + 8 > currentHeader_.length()) return 0;
    std::uint64_t lo = ReadUInt();
    std::uint64_t hi = ReadUInt();
    return (hi << 32) | lo;
}

float BIFFRecordInputStream::ReadFloat() {
    if (!currentData_ || fieldPos_ + 4 > currentHeader_.length()) return 0.0f;
    std::uint32_t v = ReadUInt();
    float f;
    std::memcpy(&f, &v, sizeof(float));
    return f;
}

double BIFFRecordInputStream::ReadDouble() {
    if (!currentData_ || fieldPos_ + 8 > currentHeader_.length()) return 0.0;
    std::uint64_t v = ReadUInt64();
    double d;
    std::memcpy(&d, &v, sizeof(double));
    return d;
}

std::size_t BIFFRecordInputStream::ReadBytes(unsigned char* out, std::size_t n) {
    if (!currentData_ || !out) return 0;
    std::size_t avail = currentHeader_.length() - fieldPos_;
    std::size_t copy = (n < avail) ? n : avail;
    std::memcpy(out, currentData_ + fieldPos_, copy);
    fieldPos_ += copy;
    return copy;
}

std::vector<unsigned char> BIFFRecordInputStream::ReadRemaining() {
    std::vector<unsigned char> result;
    if (!currentData_) return result;
    std::size_t avail = currentHeader_.length() - fieldPos_;
    result.insert(result.end(), currentData_ + fieldPos_,
                   currentData_ + fieldPos_ + avail);
    fieldPos_ += avail;
    return result;
}

void BIFFRecordInputStream::ResetField() {
    fieldPos_ = 0;
}

} // namespace biff
} // namespace office
} // namespace zq
