// =============================================================================
// src/numfmt/date_time.cpp
//
// DateTime 结构体方法实现
// =============================================================================
#include "numfmt/date_time.h"

#include <cstdio>

namespace zq {
namespace office {
namespace numfmt {

std::string DateTime::toString() const {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  year, month, day, hour, minute, second);
    return std::string(buf);
}

}  // namespace numfmt
}  // namespace office
}  // namespace zq
