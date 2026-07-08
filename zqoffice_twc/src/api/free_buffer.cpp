// =============================================================================
// src/api/free_buffer.cpp
//
// 实现 FreeStringBuffer
// =============================================================================
#include "zq/office/office.h"
#include "string_buffer.h"

extern "C" {

ZQ_OFFICE_API void ZQ_OFFICE_CALL
FreeStringBuffer(const char* buffer) {
    zq::office::api::StringBufferAllocator::instance().release(buffer);
}

} // extern "C"
