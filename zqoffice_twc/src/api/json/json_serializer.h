// =============================================================================
// src/api/json/json_serializer.h
//
// JSON 序列化：JsonValue → std::string
//
// 支持两种输出模式：
//   - compact：紧凑格式（无空白）
//   - pretty：缩进格式（2 空格缩进，便于调试）
//
// 字符串转义遵循 RFC 8259：
//   " → \"  \\ → \\  / → \/（可选）  \b → \b  \f → \f
//   \n → \n  \r → \r  \t → \t  控制字符 < 0x20 → \uXXXX
// =============================================================================
#pragma once

#include <string>

#include "json_value.h"

namespace zq {
namespace office {
namespace json {

/// 序列化选项
struct SerializeOptions {
    bool pretty = false;        ///< 是否缩进
    int indent = 2;             ///< 缩进空格数
    bool escapeSlash = false;   ///< 是否转义 /（默认不转义）
};

/// 紧凑序列化（无空白）
std::string serialize(const JsonValue& value);

/// 带选项序列化
std::string serialize(const JsonValue& value, const SerializeOptions& opts);

/// 转义字符串内容（不含外层引号）
std::string escapeString(const std::string& s, bool escapeSlash = false);

} // namespace json
} // namespace office
} // namespace zq
