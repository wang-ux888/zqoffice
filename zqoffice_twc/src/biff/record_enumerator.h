// =============================================================================
// src/biff/record_enumerator.h
//
// 记录枚举器（模板）
//
//   - 在字节流上按记录头格式迭代
//   - 模板参数 HeaderType 决定记录头格式（BIFFRecordHeader / PptRecordHeader）
//   - 支持按类型查找记录、跳过容器、嵌套迭代
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>

namespace zq {
namespace office {
namespace biff {

/// 通用记录枚举器
/// @tparam HeaderType  记录头类型（BIFFRecordHeader 或 PptRecordHeader）
template <typename HeaderType>
class RecordEnumerator {
public:
    RecordEnumerator(const unsigned char* data, std::size_t size)
        : data_(data), size_(size) {}

    /// 是否还有未读记录
    bool HasNext() const {
        if (!data_ || pos_ >= size_) return false;
        return pos_ + HeaderType::kHeaderSize <= size_;
    }

    /// 读取下一条记录
    /// @return 记录头，失败返回默认构造的空记录
    HeaderType Next() {
        HeaderType h;
        if (!HasNext()) return h;
        if (!h.Read(data_ + pos_, size_ - pos_)) return h;

        currentHeader_ = h;
        currentData_ = data_ + pos_ + HeaderType::kHeaderSize;
        currentOffset_ = pos_;
        pos_ += HeaderType::kHeaderSize + h.length();
        return h;
    }

    /// 跳过当前记录的数据部分（不推进 pos_，因为 Next() 已经推进了）
    void SkipCurrent() {
        // pos_ 已经在 Next() 中推进
    }

    /// 跳过 N 字节（用于跳过容器内的子记录）
    void Skip(std::size_t bytes) {
        pos_ += bytes;
        if (pos_ > size_) pos_ = size_;
    }

    /// 回到流起始
    void Reset() {
        pos_ = 0;
        currentOffset_ = 0;
        currentData_ = nullptr;
    }

    /// 设置读取位置
    void Seek(std::size_t offset) {
        pos_ = (offset < size_) ? offset : size_;
    }

    /// 当前记录头
    const HeaderType& currentHeader() const { return currentHeader_; }

    /// 当前记录数据指针
    const unsigned char* currentData() const { return currentData_; }

    /// 当前记录在流中的偏移
    std::size_t currentOffset() const { return currentOffset_; }

    /// 当前流位置
    std::size_t position() const { return pos_; }

    /// 流大小
    std::size_t size() const { return size_; }

    /// 查找下一条指定类型的记录
    /// @param type  要查找的记录类型
    /// @return 找到返回 true，并通过 currentHeader() 返回
    bool FindNext(typename HeaderType::Type type) {
        while (HasNext()) {
            HeaderType h = Next();
            if (h.type() == type) return true;
        }
        return false;
    }

    /// 收集所有指定类型的记录
    /// @param type  要收集的记录类型
    /// @return 记录偏移列表
    std::vector<std::size_t> CollectAll(typename HeaderType::Type type) {
        std::vector<std::size_t> result;
        Reset();
        while (HasNext()) {
            std::size_t off = pos_;
            HeaderType h;
            if (!h.Read(data_ + pos_, size_ - pos_)) break;
            if (h.type() == type) {
                result.push_back(off);
            }
            pos_ += HeaderType::kHeaderSize + h.length();
        }
        return result;
    }

private:
    const unsigned char* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t pos_ = 0;

    HeaderType currentHeader_{};
    const unsigned char* currentData_ = nullptr;
    std::size_t currentOffset_ = 0;
};

// ---------------------------------------------------------------------------
// 类型特征：为不同 HeaderType 提供 Type 字段
// 实际使用时通过 HeaderType::Type 引用，需要在 HeaderType 中定义
// BIFFRecordHeader 和 PptRecordHeader 需要添加 `using Type = ...`
// ---------------------------------------------------------------------------

} // namespace biff
} // namespace office
} // namespace zq
