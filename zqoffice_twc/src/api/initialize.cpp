// =============================================================================
// src/api/initialize.cpp
//
// 实现 ZQOfficeInitialize / ZQOfficeUninitialize
// Phase A 仅完成骨架：设置工作目录、初始化日志、注册全局状态
// 后续 Phase B-J 在此添加 OLE2/BIFF/Escher/ICU/openssl 等子系统的初始化
// =============================================================================
#include "zq/office/office.h"
#include "zq/office/types.h"
#include "zq/office/version.h"
#include "string_buffer.h"

#include <atomic>
#include <mutex>
#include <string>
#include <cstdlib>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

namespace zq {
namespace office {
namespace api {

// ---------------------------------------------------------------------------
// 全局引擎状态
// ---------------------------------------------------------------------------
struct EngineState {
    std::mutex mutex;
    std::atomic<bool> initialized{false};
    std::string workingDirectory;
    InitMode mode = InitMode::Full;
};

EngineState& engineState() {
    static EngineState s;
    return s;
}

} // namespace api
} // namespace office
} // namespace zq

// ---------------------------------------------------------------------------
// C API 实现
// ---------------------------------------------------------------------------
extern "C" {

ZQ_OFFICE_API void ZQ_OFFICE_CALL
ZQOfficeInitialize(const char* workingDirectory, int mode) {
    using namespace zq::office::api;
    auto& s = engineState();
    std::lock_guard<std::mutex> lk(s.mutex);
    if (s.initialized.load()) {
        // 已初始化，幂等返回
        return;
    }
    s.mode = static_cast<zq::office::InitMode>(mode);
    if (workingDirectory && *workingDirectory) {
        s.workingDirectory = workingDirectory;
    } else {
        // 默认使用系统临时目录
#ifdef _WIN32
        char buf[MAX_PATH + 1] = {0};
        GetTempPathA(MAX_PATH, buf);
        s.workingDirectory = buf;
#else
        const char* t = std::getenv("TMPDIR");
        s.workingDirectory = t ? t : "/tmp";
#endif
    }
    s.initialized.store(true);

    // 后续 Phase 在此添加：
    //   - ICU 加载（Phase B）
    //   - libxml2 全局初始化（Phase C）
    //   - OpenSSL 全局初始化（Phase F）
    //   - 日志系统初始化
}

ZQ_OFFICE_API void ZQ_OFFICE_CALL
ZQOfficeUninitialize() {
    using namespace zq::office::api;
    auto& s = engineState();
    std::lock_guard<std::mutex> lk(s.mutex);
    if (!s.initialized.load()) return;

    // 释放所有字符串缓冲
    StringBufferAllocator::instance().releaseAll();

    // 后续 Phase 在此添加：
    //   - OpenSSL 清理
    //   - libxml2 清理
    //   - ICU 卸载

    s.workingDirectory.clear();
    s.initialized.store(false);
}

} // extern "C"
