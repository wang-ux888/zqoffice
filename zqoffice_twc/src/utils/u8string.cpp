// =============================================================================
// src/utils/u8string.cpp
// =============================================================================
#include "u8string.h"

#include <algorithm>

namespace zq {
namespace office {

int U8String::Utf8CharBytes(unsigned char firstByte) {
    if ((firstByte & 0x80) == 0x00) return 1;       // 0xxxxxxx
    if ((firstByte & 0xE0) == 0xC0) return 2;       // 110xxxxx
    if ((firstByte & 0xF0) == 0xE0) return 3;       // 1110xxxx
    if ((firstByte & 0xF8) == 0xF0) return 4;       // 11110xxx
    return 1; // 非法首字节，按 1 字节处理
}

std::size_t U8String::CharLength(const std::string& s) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < s.size(); ) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[i]));
        if (i + n > s.size()) break; // 截断处理
        i += n;
        ++count;
    }
    return count;
}

std::string U8String::SubCharString(const std::string& s,
                                    std::size_t start,
                                    std::size_t count) {
    if (s.empty() || start == std::string::npos) return std::string();
    std::size_t byteStart = 0;
    std::size_t charIdx = 0;
    // 定位 start
    while (byteStart < s.size() && charIdx < start) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[byteStart]));
        if (byteStart + n > s.size()) break;
        byteStart += n;
        ++charIdx;
    }
    if (byteStart >= s.size()) return std::string();
    // 定位 end
    std::size_t byteEnd = byteStart;
    std::size_t cnt = 0;
    while (byteEnd < s.size() && cnt < count) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[byteEnd]));
        if (byteEnd + n > s.size()) break;
        byteEnd += n;
        ++cnt;
    }
    return s.substr(byteStart, byteEnd - byteStart);
}

std::string U8String::StripEndChar(const std::string& s, char ch) {
    if (s.empty()) return s;
    std::string result = s;
    while (!result.empty() && result.back() == ch) {
        result.pop_back();
    }
    return result;
}

std::string U8String::StripEndCharUtf8(const std::string& s,
                                       const std::string& oneChar) {
    if (s.empty() || oneChar.empty()) return s;
    std::string result = s;
    while (result.size() >= oneChar.size() &&
           result.compare(result.size() - oneChar.size(),
                          oneChar.size(), oneChar) == 0) {
        result.erase(result.size() - oneChar.size());
    }
    return result;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

int U8String::Utf8CharBytes(const char* p) {
    if (!p) return 1;
    return Utf8CharBytes(static_cast<unsigned char>(*p));
}

int U8String::CalcCharCount(const char* s, int len) {
    if (!s || len <= 0) return 0;
    int count = 0;
    int i = 0;
    while (i < len) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[i]));
        if (i + n > len) break; // 截断处理
        i += n;
        ++count;
    }
    return count;
}

int U8String::ByteToCharPos(const char* s, int len, int bytePos) {
    if (!s || len <= 0 || bytePos <= 0) return 0;
    if (bytePos >= len) return CalcCharCount(s, len);
    int charPos = 0;
    int i = 0;
    while (i < bytePos && i < len) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[i]));
        if (i + n > len) break;
        i += n;
        ++charPos;
    }
    return charPos;
}

int U8String::CharPosToByte(const char* s, int len, int charPos) {
    if (!s || len <= 0 || charPos <= 0) return 0;
    int i = 0;
    int cnt = 0;
    while (i < len && cnt < charPos) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[i]));
        if (i + n > len) break;
        i += n;
        ++cnt;
    }
    return i;
}

bool U8String::CheckIsLineBreakChar(const char* s) {
    if (!s || *s == '\0') return false;
    // \n (0x0A) / \r (0x0D) 是换行符
    return *s == '\n' || *s == '\r';
}

bool U8String::CheckValidUTF8String(const char* s) {
    if (!s) return false;
    while (*s != '\0') {
        unsigned char c = static_cast<unsigned char>(*s);
        int n = Utf8CharBytes(c);
        // 检查后续字节是否为 10xxxxxx
        for (int i = 1; i < n; ++i) {
            if (s[i] == '\0') return false;
            unsigned char cb = static_cast<unsigned char>(s[i]);
            if ((cb & 0xC0) != 0x80) return false;
        }
        s += n;
    }
    return true;
}

bool U8String::IsUtf8CharStart(const char* p) {
    if (!p || *p == '\0') return false;
    unsigned char c = static_cast<unsigned char>(*p);
    // 起始字节：0xxxxxxx / 110xxxxx / 1110xxxx / 11110xxx
    // 不是续接字节（10xxxxxx）
    return (c & 0xC0) != 0x80;
}

bool U8String::IsUtf8Char10x(const char* p) {
    if (!p || *p == '\0') return false;
    unsigned char c = static_cast<unsigned char>(*p);
    // 续接字节：10xxxxxx
    return (c & 0xC0) == 0x80;
}

std::string_view U8String::GetCharAt(std::string_view s, int idx) {
    if (idx < 0) return {};
    int charIdx = 0;
    int byteIdx = 0;
    while (byteIdx < static_cast<int>(s.size())) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[byteIdx]));
        if (byteIdx + n > static_cast<int>(s.size())) break;
        if (charIdx == idx) {
            return s.substr(byteIdx, n);
        }
        byteIdx += n;
        ++charIdx;
    }
    return {};
}

std::string_view U8String::SubCharString(const char* s, int len,
                                          int start, int count) {
    if (!s || len <= 0 || start < 0 || count <= 0) return {};
    int byteStart = 0;
    int charIdx = 0;
    // 定位 start
    while (byteStart < len && charIdx < start) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[byteStart]));
        if (byteStart + n > len) break;
        byteStart += n;
        ++charIdx;
    }
    if (byteStart >= len) return {};
    // 定位 end
    int byteEnd = byteStart;
    int cnt = 0;
    while (byteEnd < len && cnt < count) {
        int n = Utf8CharBytes(static_cast<unsigned char>(s[byteEnd]));
        if (byteEnd + n > len) break;
        byteEnd += n;
        ++cnt;
    }
    return std::string_view(s + byteStart, byteEnd - byteStart);
}

std::string_view U8String::SubCharString(std::string_view s,
                                          int start, int count) {
    return SubCharString(s.data(), static_cast<int>(s.size()),
                         start, count);
}

std::string_view U8String::StripEndChar(const char* s, int len, int count) {
    if (!s || len <= 0 || count <= 0) return {};
    // 从末尾向前跳过 count 个字符
    int byteEnd = len;
    int cnt = 0;
    while (byteEnd > 0 && cnt < count) {
        // 向前扫描找到上一个字符起始字节
        int back = 1;
        while (back < 4 && byteEnd - back >= 0) {
            unsigned char c = static_cast<unsigned char>(s[byteEnd - back]);
            if ((c & 0xC0) != 0x80) break; // 找到起始字节
            ++back;
        }
        if (byteEnd - back < 0) break;
        byteEnd -= back;
        ++cnt;
    }
    return std::string_view(s, byteEnd);
}

std::string_view U8String::StripEndChar(std::string_view s, int count) {
    return StripEndChar(s.data(), static_cast<int>(s.size()), count);
}

} // namespace office
} // namespace zq
