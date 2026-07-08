// =============================================================================
// src/version.cpp
//
// 实现 version.h 中的版本字符串接口
// =============================================================================
#include "zq/office/version.h"

#include <string>

namespace zq {
namespace office {

const char* versionString() {
    static const std::string s = std::string("ZQOffice ")
        + kVersionString
        + " (based on ZQOffice)";
    return s.c_str();
}

} // namespace office
} // namespace zq
