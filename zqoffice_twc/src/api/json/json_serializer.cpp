// =============================================================================
// src/api/json/json_serializer.cpp
// =============================================================================
#include "json_serializer.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace zq {
namespace office {
namespace json {

// ---------------------------------------------------------------------------
// 字符串转义
// ---------------------------------------------------------------------------

std::string escapeString(const std::string& s, bool escapeSlash) {
    std::string out;
    out.reserve(s.size() + 2);
    for (std::size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '/':  out += escapeSlash ? "\\/" : "/"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    // 控制字符 → \uXXXX
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// 序列化实现
// ---------------------------------------------------------------------------

namespace {

void serializeValue(const JsonValue& value, std::string& out, const SerializeOptions& opts, int depth) {
    switch (value.type()) {
        case JsonType::Null:
            out += "null";
            break;
        case JsonType::Bool:
            out += value.asBool() ? "true" : "false";
            break;
        case JsonType::Int: {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%lld",
                          static_cast<long long>(value.asInt()));
            out += buf;
            break;
        }
        case JsonType::Double: {
            double d = value.asDouble();
            if (std::isnan(d) || std::isinf(d)) {
                out += "null";  // JSON 不支持 NaN/Infinity
                break;
            }
            char buf[64];
            // 整数浮点（如 42.0 → 42.0）
            if (d == static_cast<double>(static_cast<std::int64_t>(d)) &&
                std::abs(d) < 1e15) {
                std::snprintf(buf, sizeof(buf), "%.1f", d);
                out += buf;
                break;
            }
            // 寻找最短 round-trip 表示（1..17 位有效数字）
            for (int prec = 1; prec <= 17; ++prec) {
                std::snprintf(buf, sizeof(buf), "%.*g", prec, d);
                double parsed = std::strtod(buf, nullptr);
                if (parsed == d) {
                    // 确保 JSON 数字被识别为浮点（含 . 或 e）
                    if (std::strchr(buf, '.') == nullptr &&
                        std::strchr(buf, 'e') == nullptr &&
                        std::strchr(buf, 'E') == nullptr) {
                        std::strcat(buf, ".0");
                    }
                    out += buf;
                    break;
                }
                if (prec == 17) {
                    // 兜底：17 位仍不匹配，直接使用
                    std::snprintf(buf, sizeof(buf), "%.17g", d);
                    if (std::strchr(buf, '.') == nullptr &&
                        std::strchr(buf, 'e') == nullptr &&
                        std::strchr(buf, 'E') == nullptr) {
                        std::strcat(buf, ".0");
                    }
                    out += buf;
                }
            }
            break;
        }
        case JsonType::String:
            out += '"';
            out += escapeString(value.asString(), opts.escapeSlash);
            out += '"';
            break;
        case JsonType::Array: {
            const auto& items = value.arrayItems();
            if (items.empty()) {
                out += "[]";
                break;
            }
            out += '[';
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i > 0) out += ',';
                if (opts.pretty) {
                    out += '\n';
                    for (int s = 0; s < (depth + 1) * opts.indent; ++s) out += ' ';
                }
                serializeValue(items[i], out, opts, depth + 1);
            }
            if (opts.pretty) {
                out += '\n';
                for (int s = 0; s < depth * opts.indent; ++s) out += ' ';
            }
            out += ']';
            break;
        }
        case JsonType::Object: {
            const auto& items = value.objectItems();
            if (items.empty()) {
                out += "{}";
                break;
            }
            out += '{';
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i > 0) out += ',';
                if (opts.pretty) {
                    out += '\n';
                    for (int s = 0; s < (depth + 1) * opts.indent; ++s) out += ' ';
                }
                out += '"';
                out += escapeString(items[i].first, opts.escapeSlash);
                out += '"';
                out += opts.pretty ? ": " : ":";
                serializeValue(items[i].second, out, opts, depth + 1);
            }
            if (opts.pretty) {
                out += '\n';
                for (int s = 0; s < depth * opts.indent; ++s) out += ' ';
            }
            out += '}';
            break;
        }
    }
}

} // anonymous namespace

std::string serialize(const JsonValue& value) {
    SerializeOptions opts;
    return serialize(value, opts);
}

std::string serialize(const JsonValue& value, const SerializeOptions& opts) {
    std::string out;
    serializeValue(value, out, opts, 0);
    return out;
}

} // namespace json
} // namespace office
} // namespace zq
