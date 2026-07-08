// =============================================================================
// src/api/string_buffer.cpp
// =============================================================================
#include "string_buffer.h"

#include <cstdlib>

namespace zq {
namespace office {
namespace api {

StringBufferAllocator& StringBufferAllocator::instance() {
    static StringBufferAllocator inst;
    return inst;
}

StringBufferAllocator::~StringBufferAllocator() {
    releaseAll();
}

const char* StringBufferAllocator::allocate(const char* src, std::size_t length) {
    // 用 malloc 而非 new[]，便于跨 DLL 边界释放（同一 CRT 由 CMake MT 静态链接保证）
    char* buf = static_cast<char*>(std::malloc(length + 1));
    if (!buf) return nullptr;
    if (src && length > 0) {
        std::memcpy(buf, src, length);
    }
    buf[length] = '\0';
    {
        std::lock_guard<std::mutex> lk(mutex_);
        buffers_.insert(buf);
    }
    return buf;
}

const char* StringBufferAllocator::allocate(const std::string& s) {
    return allocate(s.data(), s.size());
}

void StringBufferAllocator::release(const char* p) {
    if (!p) return;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = buffers_.find(p);
        if (it == buffers_.end()) return;
        buffers_.erase(it);
    }
    std::free(const_cast<char*>(p));
}

void StringBufferAllocator::releaseAll() {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const char* p : buffers_) {
        std::free(const_cast<char*>(p));
    }
    buffers_.clear();
}

} // namespace api
} // namespace office
} // namespace zq
