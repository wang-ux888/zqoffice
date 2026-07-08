// =============================================================================
// zq/office/version.h
//
// ZQOffice 版本信息
// =============================================================================
#pragma once

#include <cstdint>

// 主.次.补丁 版本
#define ZQ_OFFICE_VERSION_MAJOR 1
#define ZQ_OFFICE_VERSION_MINOR 0
#define ZQ_OFFICE_VERSION_PATCH 0

// 完整版本字符串
#define ZQ_OFFICE_VERSION_STRING "1.0.0"

// ABI 版本（用于 Node-API prebuild 兼容性）
#define ZQ_OFFICE_ABI_VERSION 1

// 版本号打包为整数
#define ZQ_OFFICE_VERSION \
    ((ZQ_OFFICE_VERSION_MAJOR << 16) | \
     (ZQ_OFFICE_VERSION_MINOR <<  8) | \
     (ZQ_OFFICE_VERSION_PATCH))

namespace zq {
namespace office {

inline constexpr int kVersionMajor = ZQ_OFFICE_VERSION_MAJOR;
inline constexpr int kVersionMinor = ZQ_OFFICE_VERSION_MINOR;
inline constexpr int kVersionPatch = ZQ_OFFICE_VERSION_PATCH;
inline constexpr const char* kVersionString = ZQ_OFFICE_VERSION_STRING;

/// 返回 "ZQOffice 1.0.0 (based on ZQOffice)"
const char* versionString();

} // namespace office
} // namespace zq
