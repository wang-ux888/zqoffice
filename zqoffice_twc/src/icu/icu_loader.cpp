// =============================================================================
// src/icu/icu_loader.cpp
// =============================================================================
#include "icu_loader.h"

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

#include <cstring>

namespace zq {
namespace office {
namespace icu {

IcuLoader& IcuLoader::instance() {
    static IcuLoader s;
    return s;
}

IcuLoader::IcuLoader() {}

IcuLoader::~IcuLoader() {
    unload();
}

namespace {

/// 平台无关的动态库加载
void* loadLibrary(const char* path) {
#ifdef _WIN32
    return static_cast<void*>(LoadLibraryA(path));
#else
    return dlopen(path, RTLD_LAZY);
#endif
}

/// 平台无关的符号解析
void* resolveSymbol(void* handle, const char* name) {
    if (!handle || !name) return nullptr;
#ifdef _WIN32
    HMODULE h = static_cast<HMODULE>(handle);
    return reinterpret_cast<void*>(GetProcAddress(h, name));
#else
    return dlsym(handle, name);
#endif
}

/// 平台无关的库卸载
void freeLibrary(void* handle) {
    if (!handle) return;
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

} // anonymous namespace

IcuLoadStatus IcuLoader::load() {
    if (status_ == IcuLoadStatus::Loaded) return status_;

#ifdef _WIN32
    // Windows: 尝试 icuuc.dll（ICU 64+ 命名）
    // 也尝试带版本号的命名（icuuc70.dll, icuuc71.dll 等）
    const char* candidates[] = {
        "icuuc.dll",
        "icuuc70.dll", "icuuc71.dll", "icuuc72.dll", "icuuc73.dll",
        "icuuc74.dll", "icuuc75.dll", "icuuc76.dll",
        nullptr
    };
#else
    const char* candidates[] = {
        "libicuuc.so",
        "libicuuc.so.70", "libicuuc.so.71", "libicuuc.so.72",
        "libicuuc.so.73", "libicuuc.so.74", "libicuuc.so.75",
        "libicuuc.dylib",
        nullptr
    };
#endif

    for (int i = 0; candidates[i] != nullptr; ++i) {
        handle_icuuc_ = loadLibrary(candidates[i]);
        if (handle_icuuc_) break;
    }

    if (!handle_icuuc_) {
        status_ = IcuLoadStatus::LoadFailed;
        return status_;
    }

    // 尝试加载 icuin（ICU 数据库，Windows 上独立，Linux 通常打包在 icuuc 中）
#ifdef _WIN32
    const char* dataCandidates[] = {
        "icuin.dll", "icudt.dll",
        "icuin70.dll", "icuin71.dll", "icuin72.dll", "icuin73.dll",
        "icuin74.dll", "icuin75.dll", "icuin76.dll",
        nullptr
    };
    for (int i = 0; dataCandidates[i] != nullptr; ++i) {
        handle_icuin_ = loadLibrary(dataCandidates[i]);
        if (handle_icuin_) break;
    }
    // icuin 不是必须的（某些 ICU 版本将数据嵌入 icuuc）
#endif

    if (!resolveSymbols_()) {
        freeLibrary(handle_icuuc_);
        handle_icuuc_ = nullptr;
        if (handle_icuin_) {
            freeLibrary(handle_icuin_);
            handle_icuin_ = nullptr;
        }
        status_ = IcuLoadStatus::LoadFailed;
        return status_;
    }

    status_ = IcuLoadStatus::Loaded;
    return status_;
}

bool IcuLoader::resolveSymbols_() {
    if (!handle_icuuc_) return false;

    // ucnv_* 函数
    ucnv_open_           = reinterpret_cast<UcnvOpenFn>(resolveSymbol(handle_icuuc_, "ucnv_open"));
    ucnv_close_          = reinterpret_cast<UcnvCloseFn>(resolveSymbol(handle_icuuc_, "ucnv_close"));
    ucnv_toUnicode_      = reinterpret_cast<UcnvToUnicodeFn>(resolveSymbol(handle_icuuc_, "ucnv_toUChars"));
    ucnv_fromUnicode_    = reinterpret_cast<UcnvFromUnicodeFn>(resolveSymbol(handle_icuuc_, "ucnv_fromUChars"));
    ucnv_getDefaultName_ = reinterpret_cast<UcnvGetDefaultNameFn>(resolveSymbol(handle_icuuc_, "ucnv_getDefaultName"));

    // ucsdet_* 函数（在 icuin.dll 或 libicui18n 中）
    void* i18nHandle = handle_icuin_ ? handle_icuin_ : handle_icuuc_;
#ifdef _WIN32
    // Windows: ucsdet 在 icuin.dll 中
    i18nHandle = handle_icuin_ ? handle_icuin_ : handle_icuuc_;
#else
    // Linux: 尝试 libicui18n
    if (!handle_icuin_) {
        handle_icuin_ = loadLibrary("libicui18n.so");
        i18nHandle = handle_icuin_ ? handle_icuin_ : handle_icuuc_;
    }
#endif

    ucsdet_open_          = reinterpret_cast<UcsdetOpenFn>(resolveSymbol(i18nHandle, "ucsdet_open"));
    ucsdet_close_         = reinterpret_cast<UcsdetCloseFn>(resolveSymbol(i18nHandle, "ucsdet_close"));
    ucsdet_setText_       = reinterpret_cast<UcsdetSetTextFn>(resolveSymbol(i18nHandle, "ucsdet_setText"));
    ucsdet_detect_        = reinterpret_cast<UcsdetDetectFn>(resolveSymbol(i18nHandle, "ucsdet_detect"));
    ucsdet_getName_       = reinterpret_cast<UcsdetGetNameFn>(resolveSymbol(i18nHandle, "ucsdet_getName"));
    ucsdet_getConfidence_ = reinterpret_cast<UcsdetGetConfidenceFn>(resolveSymbol(i18nHandle, "ucsdet_getConfidence"));

    // 至少 ucnv_open 和 ucnv_close 必须可用
    return ucnv_open_ != nullptr && ucnv_close_ != nullptr;
}

void IcuLoader::unload() {
    if (handle_icuuc_) {
        freeLibrary(handle_icuuc_);
        handle_icuuc_ = nullptr;
    }
    if (handle_icuin_) {
        freeLibrary(handle_icuin_);
        handle_icuin_ = nullptr;
    }

    ucnv_open_           = nullptr;
    ucnv_close_          = nullptr;
    ucnv_toUnicode_      = nullptr;
    ucnv_fromUnicode_    = nullptr;
    ucnv_getDefaultName_ = nullptr;
    ucsdet_open_         = nullptr;
    ucsdet_close_        = nullptr;
    ucsdet_setText_      = nullptr;
    ucsdet_detect_       = nullptr;
    ucsdet_getName_      = nullptr;
    ucsdet_getConfidence_ = nullptr;

    status_ = IcuLoadStatus::NotLoaded;
}

const char* IcuLoader::version() const {
    if (!isLoaded()) return "ICU not loaded";
    // ICU 没有简单的版本查询函数指针，返回占位字符串
    return "ICU (dynamic)";
}

} // namespace icu
} // namespace office
} // namespace zq
