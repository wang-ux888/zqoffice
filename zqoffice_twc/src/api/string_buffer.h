// =============================================================================
// src/api/string_buffer.h
//
// ZQOffice C API 字符串缓冲管理
//
// 与 ZQOffice FreeStringBuffer 对齐：
//   所有 XxxxToZQYyy / ZQYyyToXxx C API 返回的 const char* 字符串
//   均由 StringBufferAllocator 分配，调用方需通过 FreeStringBuffer 释放。
// =============================================================================
#pragma once

#include <cstddef>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_set>

#include "zq/office/export.h"

namespace zq {
namespace office {
namespace api {

/// 字符串缓冲分配器（线程安全）
/// 所有 C API 返回的字符串均通过此分配器分配，
/// 调用方通过 FreeStringBuffer 释放。
class StringBufferAllocator {
public:
    static StringBufferAllocator& instance();

    /// 分配 length+1 字节，复制 src 内容，并追加 '\0'
    /// @return 返回的指针调用方不可 free，必须通过 FreeStringBuffer 释放
    const char* allocate(const char* src, std::size_t length);

    /// 分配并以 std::string 内容初始化
    const char* allocate(const std::string& s);

    /// 释放由 allocate 返回的指针
    /// @param p  传入 nullptr 是安全空操作
    void release(const char* p);

    /// 释放所有缓冲（仅用于引擎反初始化）
    void releaseAll();

private:
    StringBufferAllocator() = default;
    ~StringBufferAllocator();
    StringBufferAllocator(const StringBufferAllocator&) = delete;
    StringBufferAllocator& operator=(const StringBufferAllocator&) = delete;

    std::mutex mutex_;
    std::unordered_set<const char*> buffers_;
};

} // namespace api
} // namespace office
} // namespace zq
