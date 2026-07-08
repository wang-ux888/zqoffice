// =============================================================================
// src/crypto/internal/base64.cpp
//
// Base64 编码/解码（RFC 4648）
// =============================================================================
#include "base64.h"

namespace zq {
namespace office {
namespace crypto {
namespace internal {

namespace {

constexpr char kEncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/// 解码表（运行时构建，避免静态初始化顺序问题）
struct DecodeTable {
    int8_t t[256];
    DecodeTable() {
        for (int i = 0; i < 256; ++i) t[i] = -1;
        for (int i = 0; i < 26; ++i) t['A' + i] = static_cast<int8_t>(i);
        for (int i = 0; i < 26; ++i) t['a' + i] = static_cast<int8_t>(26 + i);
        for (int i = 0; i < 10; ++i) t['0' + i] = static_cast<int8_t>(52 + i);
        t['+'] = 62;
        t['/'] = 63;
    }
};

const DecodeTable& getDecodeTable() {
    static DecodeTable t;
    return t;
}

} // namespace

std::string base64Encode(const void* data, std::size_t len) {
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    std::size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        std::uint32_t v = (static_cast<std::uint32_t>(p[i]) << 16) |
                          (static_cast<std::uint32_t>(p[i + 1]) << 8) |
                          static_cast<std::uint32_t>(p[i + 2]);
        out.push_back(kEncodeTable[(v >> 18) & 0x3F]);
        out.push_back(kEncodeTable[(v >> 12) & 0x3F]);
        out.push_back(kEncodeTable[(v >> 6) & 0x3F]);
        out.push_back(kEncodeTable[v & 0x3F]);
    }

    if (i < len) {
        std::uint32_t v = static_cast<std::uint32_t>(p[i]) << 16;
        if (i + 1 < len) v |= static_cast<std::uint32_t>(p[i + 1]) << 8;
        out.push_back(kEncodeTable[(v >> 18) & 0x3F]);
        out.push_back(kEncodeTable[(v >> 12) & 0x3F]);
        if (i + 1 < len) {
            out.push_back(kEncodeTable[(v >> 6) & 0x3F]);
            out.push_back('=');  // 1 字节剩余 → 2 个 =
        } else {
            out.push_back('=');
            out.push_back('=');  // 2 字节剩余 → 1 个 =
        }
    }
    return out;
}

std::string base64Encode(const std::string& data) {
    return base64Encode(data.data(), data.size());
}

std::string base64Decode(const std::string& encoded) {
    const auto& tbl = getDecodeTable();
    std::string out;
    out.reserve(encoded.size() * 3 / 4);

    std::uint32_t buf = 0;
    int bits = 0;
    for (char c : encoded) {
        if (c == '=') break;
        int v = tbl.t[static_cast<unsigned char>(c)];
        if (v < 0) continue;  // 跳过空白等非法字符
        buf = (buf << 6) | static_cast<std::uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

} // namespace internal
} // namespace crypto
} // namespace office
} // namespace zq
