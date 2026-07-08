// =============================================================================
// src/icu/icu_loader.h
//
// ICU 动态加载器
//
//   1. 启动时尝试加载 ICU 共享库（icuuc.dll / libicuuc.so）
//   2. 加载成功则解析所有需要的函数指针（ucsdet_* / ucnv_*）
//   3. 加载失败则 CharsetDetector/Converter 退化为内置实现
//
//   优点：
//   - 不强制 ICU 运行时依赖，便于分发
//   - 当 ICU 可用时自动启用完整编码检测/转换能力
//   - 当 ICU 不可用时仍可处理常见编码（UTF-8/UTF-16/Latin-1）
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace zq {
namespace office {
namespace icu {

/// ICU 加载状态
enum class IcuLoadStatus {
    NotLoaded,       ///< 未加载
    Loaded,          ///< 已成功加载 ICU
    LoadFailed,      ///< 加载失败（库不存在或符号缺失）
};

/// ICU 动态加载器（单例）
class IcuLoader {
public:
    /// 获取单例
    static IcuLoader& instance();

    /// 尝试加载 ICU 共享库
    /// @return 加载结果状态
    IcuLoadStatus load();

    /// 是否已加载 ICU
    bool isLoaded() const { return status_ == IcuLoadStatus::Loaded; }

    /// 当前状态
    IcuLoadStatus status() const { return status_; }

    /// 卸载 ICU（清理资源）
    void unload();

    /// ICU 版本字符串（仅当 isLoaded() 时有效）
    const char* version() const;

    // -----------------------------------------------------------------------
    // ICU 函数指针类型定义（仅当 isLoaded() 时有效）
    // -----------------------------------------------------------------------

    // ucnv_* (编码转换)
    typedef void* (*UcnvOpenFn)(const char* converterName, int* err);
    typedef void (*UcnvCloseFn)(void* converter);
    typedef int32_t (*UcnvToUnicodeFn)(void* converter,
                                        char16_t* dest, int32_t destCapacity,
                                        const char** src, const char* srcLimit,
                                        const int32_t* offsets, int32_t* err);
    typedef int32_t (*UcnvFromUnicodeFn)(void* converter,
                                          char* dest, int32_t destCapacity,
                                          const char16_t** src, const char16_t* srcLimit,
                                          const int32_t* offsets, int32_t* err);
    typedef const char* (*UcnvGetDefaultNameFn)(void);
    typedef void (*UcnvSetDefaultNameFn)(const char* name);

    // ucsdet_* (编码检测)
    typedef void* (*UcsdetOpenFn)(int* err);
    typedef void (*UcsdetCloseFn)(void* ucsdet);
    typedef void (*UcsdetSetTextFn)(void* ucsdet, const char* text, int32_t len, int* err);
    typedef void* (*UcsdetDetectFn)(void* ucsdet, int* err);
    typedef const char* (*UcsdetGetNameFn)(void* ucsdet, int* err);
    typedef int32_t (*UcsdetGetConfidenceFn)(void* ucsdet, int* err);

    // 函数指针访问器（返回 nullptr 当未加载时）
    UcnvOpenFn           ucnv_open() const           { return ucnv_open_; }
    UcnvCloseFn          ucnv_close() const          { return ucnv_close_; }
    UcnvToUnicodeFn      ucnv_toUnicode() const      { return ucnv_toUnicode_; }
    UcnvFromUnicodeFn    ucnv_fromUnicode() const    { return ucnv_fromUnicode_; }
    UcnvGetDefaultNameFn ucnv_getDefaultName() const { return ucnv_getDefaultName_; }
    UcsdetOpenFn         ucsdet_open() const         { return ucsdet_open_; }
    UcsdetCloseFn        ucsdet_close() const        { return ucsdet_close_; }
    UcsdetSetTextFn      ucsdet_setText() const      { return ucsdet_setText_; }
    UcsdetDetectFn       ucsdet_detect() const       { return ucsdet_detect_; }
    UcsdetGetNameFn      ucsdet_getName() const      { return ucsdet_getName_; }
    UcsdetGetConfidenceFn ucsdet_getConfidence() const { return ucsdet_getConfidence_; }

private:
    IcuLoader();
    ~IcuLoader();
    IcuLoader(const IcuLoader&) = delete;
    IcuLoader& operator=(const IcuLoader&) = delete;

    IcuLoadStatus status_ = IcuLoadStatus::NotLoaded;

    // 平台句柄
#ifdef _WIN32
    void* handle_icuuc_ = nullptr;  // HMODULE
    void* handle_icuin_ = nullptr;  // HMODULE（ICU 数据库）
#else
    void* handle_icuuc_ = nullptr;  // dlopen 句柄
    void* handle_icuin_ = nullptr;
#endif

    // 函数指针
    UcnvOpenFn            ucnv_open_           = nullptr;
    UcnvCloseFn           ucnv_close_          = nullptr;
    UcnvToUnicodeFn       ucnv_toUnicode_      = nullptr;
    UcnvFromUnicodeFn     ucnv_fromUnicode_    = nullptr;
    UcnvGetDefaultNameFn  ucnv_getDefaultName_ = nullptr;
    UcsdetOpenFn          ucsdet_open_         = nullptr;
    UcsdetCloseFn         ucsdet_close_        = nullptr;
    UcsdetSetTextFn       ucsdet_setText_      = nullptr;
    UcsdetDetectFn        ucsdet_detect_       = nullptr;
    UcsdetGetNameFn       ucsdet_getName_      = nullptr;
    UcsdetGetConfidenceFn ucsdet_getConfidence_ = nullptr;

    /// 解析所有函数符号
    /// @return 全部解析成功返回 true
    bool resolveSymbols_();
};

} // namespace icu
} // namespace office
} // namespace zq
