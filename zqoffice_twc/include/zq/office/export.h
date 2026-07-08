// =============================================================================
// zq/office/export.h
//
// DLL 导出宏定义
//   - Windows：__declspec(dllexport/dllimport)
//   - macOS/Linux：visibility 属性
// =============================================================================
#pragma once

// 由构建系统（CMake/binding.gyp）在编译 zqoffice 时定义
// 在使用方（链接到 zqoffice_core 的客户端）则不定义

#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(ZQ_OFFICE_BUILDING_DLL)
    // 正在编译 zqoffice.dll
    #define ZQ_OFFICE_API __declspec(dllexport)
  #elif defined(ZQ_OFFICE_USING_DLL)
    // 使用方导入
    #define ZQ_OFFICE_API __declspec(dllimport)
  #else
    // 静态库场景
    #define ZQ_OFFICE_API
  #endif
  #define ZQ_OFFICE_LOCAL
#else
  #if __GNUC__ >= 4
    #define ZQ_OFFICE_API __attribute__((visibility("default")))
    #define ZQ_OFFICE_LOCAL __attribute__((visibility("hidden")))
  #else
    #define ZQ_OFFICE_API
    #define ZQ_OFFICE_LOCAL
  #endif
#endif

// C ABI 兼容宏
#if defined(__cplusplus)
  #define ZQ_OFFICE_EXTERN_C extern "C"
#else
  #define ZQ_OFFICE_EXTERN_C
#endif

#define ZQ_OFFICE_C_API ZQ_OFFICE_EXTERN_C ZQ_OFFICE_API

#if defined(_WIN32) && !defined(ZQ_OFFICE_NO_CDECL)
  #define ZQ_OFFICE_CALL __cdecl
#else
  #define ZQ_OFFICE_CALL
#endif
