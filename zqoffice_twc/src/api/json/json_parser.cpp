// =============================================================================
// src/api/json/json_parser.cpp
//
// 递归下降 JSON 解析器
// =============================================================================
#include "json_parser.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace zq {
namespace office {
namespace json {

namespace {

/// 解析器上下文
class Parser {
public:
    Parser(const char* text, std::size_t length)
        : text_(text), length_(length), pos_(0), line_(1), col_(1) {}

    /// 主解析入口（解析一个值，跳过前后空白）
    JsonValue parse() {
        skipWhitespace();
        JsonValue v = parseValue();
        skipWhitespace();
        if (pos_ < length_) {
            throwError("trailing characters after JSON value");
        }
        return v;
    }

private:
    // -----------------------------------------------------------------------
    // 辅助
    // -----------------------------------------------------------------------

    char peek() const {
        if (pos_ >= length_) return '\0';
        return text_[pos_];
    }

    char next() {
        if (pos_ >= length_) return '\0';
        char c = text_[pos_++];
        if (c == '\n') {
            ++line_;
            col_ = 1;
        } else {
            ++col_;
        }
        return c;
    }

    bool match(char expected) {
        if (peek() == expected) {
            next();
            return true;
        }
        return false;
    }

    void expect(char expected) {
        if (!match(expected)) {
            throwError(std::string("expected '") + expected + "'");
        }
    }

    [[noreturn]] void throwError(const std::string& msg) {
        throw JsonParseError(msg, line_, col_);
    }

    void skipWhitespace() {
        while (pos_ < length_) {
            char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                next();
            } else {
                break;
            }
        }
    }

    // -----------------------------------------------------------------------
    // 值解析
    // -----------------------------------------------------------------------

    JsonValue parseValue() {
        skipWhitespace();
        if (pos_ >= length_) throwError("unexpected end of input");

        char c = peek();
        switch (c) {
            case '{': return parseObject();
            case '[': return parseArray();
            case '"': return parseString();
            case 't': case 'f': return parseBool();
            case 'n': return parseNull();
            case '-': case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7': case '8':
            case '9': case '+': case '.':
                return parseNumber();
            default:
                throwError(std::string("unexpected character '") + c + "'");
        }
    }

    JsonValue parseObject() {
        expect('{');
        JsonValue obj = JsonValue::object();
        skipWhitespace();
        if (match('}')) return obj;

        while (true) {
            skipWhitespace();
            // 键必须是字符串
            if (peek() != '"') throwError("expected string key in object");
            std::string key = parseStringRaw();

            skipWhitespace();
            expect(':');

            JsonValue value = parseValue();
            obj.set(key, std::move(value));

            skipWhitespace();
            if (match('}')) break;
            expect(',');
        }
        return obj;
    }

    JsonValue parseArray() {
        expect('[');
        JsonValue arr = JsonValue::array();
        skipWhitespace();
        if (match(']')) return arr;

        while (true) {
            JsonValue value = parseValue();
            arr.append(std::move(value));

            skipWhitespace();
            if (match(']')) break;
            expect(',');
        }
        return arr;
    }

    JsonValue parseString() {
        return JsonValue(parseStringRaw());
    }

    std::string parseStringRaw() {
        expect('"');
        std::string out;
        while (true) {
            if (pos_ >= length_) throwError("unterminated string");
            char c = next();
            if (c == '"') break;
            if (c == '\\') {
                // 转义
                if (pos_ >= length_) throwError("unterminated escape");
                char esc = next();
                switch (esc) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'u': {
                        // \uXXXX → UTF-8 编码
                        unsigned int cp = parseUnicodeEscape();
                        encodeUtf8(cp, out);
                        break;
                    }
                    default:
                        throwError(std::string("invalid escape '\\") + esc + "'");
                }
            } else if (static_cast<unsigned char>(c) < 0x20) {
                throwError("unescaped control character in string");
            } else {
                out += c;
            }
        }
        return out;
    }

    unsigned int parseUnicodeEscape() {
        // 4 位十六进制
        unsigned int cp = 0;
        for (int i = 0; i < 4; ++i) {
            if (pos_ >= length_) throwError("unterminated \\u escape");
            char c = next();
            int digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
            else throwError(std::string("invalid hex digit '") + c + "'");
            cp = (cp << 4) | static_cast<unsigned int>(digit);
        }
        // 处理代理对
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            // 高代理项，期望跟一个 \uXXXX 低代理项
            if (pos_ + 1 < length_ && text_[pos_] == '\\' && text_[pos_ + 1] == 'u') {
                next(); // '\'
                next(); // 'u'
                unsigned int low = parseUnicodeEscape();
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                } else {
                    throwError("invalid low surrogate");
                }
            } else {
                throwError("missing low surrogate");
            }
        }
        return cp;
    }

    static void encodeUtf8(unsigned int cp, std::string& out) {
        if (cp <= 0x7F) {
            out += static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0x10FFFF) {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        // 超出范围的码点忽略（理论上 parseUnicodeEscape 不会产生）
    }

    JsonValue parseBool() {
        if (matchKeyword("true")) return JsonValue(true);
        if (matchKeyword("false")) return JsonValue(false);
        throwError("invalid literal");
    }

    JsonValue parseNull() {
        if (matchKeyword("null")) return JsonValue();
        throwError("invalid literal");
    }

    bool matchKeyword(const char* kw) {
        std::size_t len = std::strlen(kw);
        if (pos_ + len > length_) return false;
        for (std::size_t i = 0; i < len; ++i) {
            if (text_[pos_ + i] != kw[i]) return false;
        }
        // 检查后继字符不是字母/数字（避免 trueXYZ 误匹配）
        if (pos_ + len < length_) {
            char next_char = text_[pos_ + len];
            if (std::isalnum(static_cast<unsigned char>(next_char)) || next_char == '_') {
                return false;
            }
        }
        for (std::size_t i = 0; i < len; ++i) next();
        return true;
    }

    JsonValue parseNumber() {
        // 收集数字字符串
        std::string numStr;
        bool isFloat = false;

        // 可选正负号
        if (peek() == '-' || peek() == '+') {
            numStr += next();
        }

        // 整数部分
        if (peek() == '0') {
            numStr += next();
        } else if (peek() >= '1' && peek() <= '9') {
            while (pos_ < length_ && std::isdigit(static_cast<unsigned char>(peek()))) {
                numStr += next();
            }
        } else if (peek() == '.') {
            isFloat = true;
        } else {
            throwError("invalid number");
        }

        // 小数部分
        if (peek() == '.') {
            isFloat = true;
            numStr += next();
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                throwError("digit expected after decimal point");
            }
            while (pos_ < length_ && std::isdigit(static_cast<unsigned char>(peek()))) {
                numStr += next();
            }
        }

        // 指数部分
        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            numStr += next();
            if (peek() == '+' || peek() == '-') {
                numStr += next();
            }
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                throwError("digit expected in exponent");
            }
            while (pos_ < length_ && std::isdigit(static_cast<unsigned char>(peek()))) {
                numStr += next();
            }
        }

        if (isFloat) {
            return JsonValue(std::strtod(numStr.c_str(), nullptr));
        } else {
            // 尝试 int64
            try {
                return JsonValue(static_cast<std::int64_t>(std::strtoll(numStr.c_str(), nullptr, 10)));
            } catch (...) {
                // 溢出则降级为 double
                return JsonValue(std::strtod(numStr.c_str(), nullptr));
            }
        }
    }

    const char* text_;
    std::size_t length_;
    std::size_t pos_;
    std::size_t line_;
    std::size_t col_;
};

} // anonymous namespace

JsonValue parse(const std::string& text) {
    Parser p(text.data(), text.size());
    return p.parse();
}

JsonValue parse(const char* text, std::size_t length) {
    Parser p(text, length);
    return p.parse();
}

} // namespace json
} // namespace office
} // namespace zq
