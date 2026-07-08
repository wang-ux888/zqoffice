// =============================================================================
// src/api/json/json_parser.h
//
// JSON 解析：std::string → JsonValue
//
// 遵循 RFC 8259：
//   - 支持 7 种类型
//   - 字符串支持转义（\" \\ \/ \b \f \n \r \t \uXXXX）
//   - 数字支持整数/浮点/科学计数法/负号
//   - 支持前后空白（space/tab/CR/LF）
//   - UTF-8 字符串原样保留（不解码为 UTF-32）
//
// 错误处理：
//   - 解析失败抛出 JsonParseError 异常
//   - 错误信息包含位置（行/列）
// =============================================================================
#pragma once

#include <stdexcept>
#include <string>

#include "json_value.h"

namespace zq {
namespace office {
namespace json {

/// JSON 解析错误异常
class JsonParseError : public std::runtime_error {
public:
    JsonParseError(const std::string& msg, std::size_t line, std::size_t col)
        : std::runtime_error(msg + " (line " + std::to_string(line) +
                             ", col " + std::to_string(col) + ")")
        , line_(line)
        , col_(col) {}
    std::size_t line() const { return line_; }
    std::size_t col() const { return col_; }
private:
    std::size_t line_;
    std::size_t col_;
};

/// 解析 JSON 字符串
/// @throws JsonParseError 解析失败时抛出
JsonValue parse(const std::string& text);

/// 解析 JSON 字符串（C 风格）
JsonValue parse(const char* text, std::size_t length);

} // namespace json
} // namespace office
} // namespace zq
