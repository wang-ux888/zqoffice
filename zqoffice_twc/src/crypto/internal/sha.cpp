// =============================================================================
// src/crypto/internal/sha.cpp
//
// SHA-1 / SHA-256 / SHA-384 / SHA-512 / HMAC-SHA1 / HMAC-SHA512 实现
//
// 基于 FIPS 180-4 规范的纯 C++ 实现。
// =============================================================================
#include "sha.h"

#include <cstring>
#include <utility>
#include <vector>

namespace zq {
namespace office {
namespace crypto {
namespace internal {

// ---------------------------------------------------------------------------
// 工具函数
// ---------------------------------------------------------------------------
namespace {

/// 循环左移（32 位）
inline std::uint32_t rotl32(std::uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

/// 循环右移（32 位）
inline std::uint32_t rotr32(std::uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

/// 循环右移（64 位）
inline std::uint64_t rotr64(std::uint64_t x, int n) {
    return (x >> n) | (x << (64 - n));
}

/// 大端写入 32 位
inline void storeBe32(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v >> 24);
    p[1] = static_cast<std::uint8_t>(v >> 16);
    p[2] = static_cast<std::uint8_t>(v >> 8);
    p[3] = static_cast<std::uint8_t>(v);
}

/// 大端写入 64 位
inline void storeBe64(std::uint8_t* p, std::uint64_t v) {
    p[0] = static_cast<std::uint8_t>(v >> 56);
    p[1] = static_cast<std::uint8_t>(v >> 48);
    p[2] = static_cast<std::uint8_t>(v >> 40);
    p[3] = static_cast<std::uint8_t>(v >> 32);
    p[4] = static_cast<std::uint8_t>(v >> 24);
    p[5] = static_cast<std::uint8_t>(v >> 16);
    p[6] = static_cast<std::uint8_t>(v >> 8);
    p[7] = static_cast<std::uint8_t>(v);
}

/// 大端读取 32 位
inline std::uint32_t loadBe32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8)  |
           (static_cast<std::uint32_t>(p[3]));
}

/// 大端读取 64 位
inline std::uint64_t loadBe64(const std::uint8_t* p) {
    return (static_cast<std::uint64_t>(p[0]) << 56) |
           (static_cast<std::uint64_t>(p[1]) << 48) |
           (static_cast<std::uint64_t>(p[2]) << 40) |
           (static_cast<std::uint64_t>(p[3]) << 32) |
           (static_cast<std::uint64_t>(p[4]) << 24) |
           (static_cast<std::uint64_t>(p[5]) << 16) |
           (static_cast<std::uint64_t>(p[6]) << 8)  |
           (static_cast<std::uint64_t>(p[7]));
}

} // namespace

// ===========================================================================
// SHA-1（FIPS 180-4 §6.1）
// ===========================================================================
namespace {

constexpr std::uint32_t kSha1H[5] = {
    0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};

constexpr std::uint32_t kSha1K[4] = {
    0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
};

/// SHA-1 块处理（512 bit = 64 字节块）
void sha1ProcessBlock(std::uint32_t h[5], const std::uint8_t* block) {
    std::uint32_t w[80];
    for (int t = 0; t < 16; ++t) {
        w[t] = loadBe32(block + t * 4);
    }
    for (int t = 16; t < 80; ++t) {
        w[t] = rotl32(w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16], 1);
    }

    std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int t = 0; t < 80; ++t) {
        std::uint32_t f, k;
        if (t < 20) {
            f = (b & c) | ((~b) & d);
            k = kSha1K[0];
        } else if (t < 40) {
            f = b ^ c ^ d;
            k = kSha1K[1];
        } else if (t < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = kSha1K[2];
        } else {
            f = b ^ c ^ d;
            k = kSha1K[3];
        }
        std::uint32_t temp = rotl32(a, 5) + f + e + k + w[t];
        e = d;
        d = c;
        c = rotl32(b, 30);
        b = a;
        a = temp;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

} // namespace

void sha1(const void* data, std::size_t len, std::uint8_t out[20]) {
    std::uint32_t h[5];
    for (int i = 0; i < 5; ++i) h[i] = kSha1H[i];

    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t remaining = len;

    // 处理完整块（64 字节）
    while (remaining >= 64) {
        sha1ProcessBlock(h, p);
        p += 64;
        remaining -= 64;
    }

    // 最后一块（含 padding）
    std::uint8_t last[128] = {0};
    std::memcpy(last, p, remaining);
    last[remaining] = 0x80;

    std::size_t lastLen;
    if (remaining >= 56) {
        lastLen = 128;
    } else {
        lastLen = 64;
    }
    // 长度（位）大端写入末尾 8 字节
    std::uint64_t bitLen = static_cast<std::uint64_t>(len) * 8;
    storeBe64(last + lastLen - 8, bitLen);

    sha1ProcessBlock(h, last);
    if (lastLen == 128) {
        sha1ProcessBlock(h, last + 64);
    }

    for (int i = 0; i < 5; ++i) {
        storeBe32(out + i * 4, h[i]);
    }
}

std::string sha1(const std::string& data) {
    std::uint8_t out[20];
    sha1(data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 20);
}

// ===========================================================================
// SHA-256（FIPS 180-4 §6.2）
// ===========================================================================
namespace {

constexpr std::uint32_t kSha256H[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

constexpr std::uint32_t kSha256K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void sha256ProcessBlock(std::uint32_t h[8], const std::uint8_t* block) {
    std::uint32_t w[64];
    for (int t = 0; t < 16; ++t) {
        w[t] = loadBe32(block + t * 4);
    }
    for (int t = 16; t < 64; ++t) {
        std::uint32_t s0 = rotr32(w[t - 15], 7) ^ rotr32(w[t - 15], 18) ^ (w[t - 15] >> 3);
        std::uint32_t s1 = rotr32(w[t - 2], 17) ^ rotr32(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }

    std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int t = 0; t < 64; ++t) {
        std::uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        std::uint32_t ch = (e & f) ^ ((~e) & g);
        std::uint32_t temp1 = hh + S1 + ch + kSha256K[t] + w[t];
        std::uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        std::uint32_t temp2 = S0 + maj;
        hh = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

} // namespace

void sha256(const void* data, std::size_t len, std::uint8_t out[32]) {
    std::uint32_t h[8];
    for (int i = 0; i < 8; ++i) h[i] = kSha256H[i];

    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t remaining = len;

    while (remaining >= 64) {
        sha256ProcessBlock(h, p);
        p += 64;
        remaining -= 64;
    }

    std::uint8_t last[128] = {0};
    std::memcpy(last, p, remaining);
    last[remaining] = 0x80;

    std::size_t lastLen;
    if (remaining >= 56) {
        lastLen = 128;
    } else {
        lastLen = 64;
    }
    std::uint64_t bitLen = static_cast<std::uint64_t>(len) * 8;
    storeBe64(last + lastLen - 8, bitLen);

    sha256ProcessBlock(h, last);
    if (lastLen == 128) {
        sha256ProcessBlock(h, last + 64);
    }

    for (int i = 0; i < 8; ++i) {
        storeBe32(out + i * 4, h[i]);
    }
}

std::string sha256(const std::string& data) {
    std::uint8_t out[32];
    sha256(data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 32);
}

// ===========================================================================
// SHA-512（FIPS 180-4 §6.4） / SHA-384（§6.3）
// ===========================================================================
namespace {

constexpr std::uint64_t kSha512H[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

constexpr std::uint64_t kSha384H[8] = {
    0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL,
    0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
    0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
    0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL
};

constexpr std::uint64_t kSha512K[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

void sha512ProcessBlock(std::uint64_t h[8], const std::uint8_t* block) {
    std::uint64_t w[80];
    for (int t = 0; t < 16; ++t) {
        w[t] = loadBe64(block + t * 8);
    }
    for (int t = 16; t < 80; ++t) {
        std::uint64_t s0 = rotr64(w[t - 15], 1) ^ rotr64(w[t - 15], 8) ^ (w[t - 15] >> 7);
        std::uint64_t s1 = rotr64(w[t - 2], 19) ^ rotr64(w[t - 2], 61) ^ (w[t - 2] >> 6);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }

    std::uint64_t a = h[0], b = h[1], c = h[2], d = h[3];
    std::uint64_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int t = 0; t < 80; ++t) {
        std::uint64_t S1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
        std::uint64_t ch = (e & f) ^ ((~e) & g);
        std::uint64_t temp1 = hh + S1 + ch + kSha512K[t] + w[t];
        std::uint64_t S0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
        std::uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        std::uint64_t temp2 = S0 + maj;
        hh = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

/// SHA-512 / SHA-384 共用核心（输入 H 初值不同，输出长度不同）
void sha512Core(const std::uint64_t hInit[8],
                const void* data, std::size_t len,
                std::uint8_t* out, std::size_t outLen) {
    std::uint64_t h[8];
    for (int i = 0; i < 8; ++i) h[i] = hInit[i];

    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t remaining = len;

    // SHA-512 块大小 = 128 字节
    while (remaining >= 128) {
        sha512ProcessBlock(h, p);
        p += 128;
        remaining -= 128;
    }

    // 最后一块（含 padding，128 字节对齐）
    // SHA-512 用 128 位（16 字节）长度字段，所以 padding 后总长需 ≥ 112 字节才能容纳 16 字节长度
    std::uint8_t last[256] = {0};
    std::memcpy(last, p, remaining);
    last[remaining] = 0x80;

    std::size_t lastLen;
    if (remaining >= 112) {
        lastLen = 256;
    } else {
        lastLen = 128;
    }
    std::uint64_t bitLen = static_cast<std::uint64_t>(len) * 8;
    // 128 位长度（高 64 位为 0，低 64 位为 bitLen）
    storeBe64(last + lastLen - 8, bitLen);  // 低 64 位
    // 高 64 位（仅当 len 极大时非 0，正常情况下为 0）

    sha512ProcessBlock(h, last);
    if (lastLen == 256) {
        sha512ProcessBlock(h, last + 128);
    }

    // 输出（大端）
    for (std::size_t i = 0; i < outLen; ++i) {
        out[i] = static_cast<std::uint8_t>(h[i / 8] >> (56 - (i % 8) * 8));
    }
}

} // namespace

void sha512(const void* data, std::size_t len, std::uint8_t out[64]) {
    sha512Core(kSha512H, data, len, out, 64);
}

std::string sha512(const std::string& data) {
    std::uint8_t out[64];
    sha512(data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 64);
}

void sha384(const void* data, std::size_t len, std::uint8_t out[48]) {
    sha512Core(kSha384H, data, len, out, 48);
}

std::string sha384(const std::string& data) {
    std::uint8_t out[48];
    sha384(data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 48);
}

// ===========================================================================
// HMAC（FIPS 198 / RFC 2104）
// ===========================================================================

/// HMAC 通用实现（块大小可变，用于 SHA-1/SHA-256/SHA-512）
template <std::size_t BlockSize, std::size_t DigestSize>
static void hmacGeneric(
    const void* key, std::size_t keyLen,
    const void* data, std::size_t dataLen,
    std::uint8_t out[DigestSize],
    void (*hashFunc)(const void*, std::size_t, std::uint8_t*)) {

    std::uint8_t k[BlockSize] = {0};
    if (keyLen <= BlockSize) {
        std::memcpy(k, key, keyLen);
    } else {
        // 长密钥先 hash，结果填入 k
        hashFunc(key, keyLen, k);
    }

    // ipad / opad
    std::uint8_t ipad[BlockSize];
    std::uint8_t opad[BlockSize];
    for (std::size_t i = 0; i < BlockSize; ++i) {
        ipad[i] = k[i] ^ 0x36;
        opad[i] = k[i] ^ 0x5c;
    }

    // inner = H(ipad || data)
    std::uint8_t inner[DigestSize];
    // 拼接 ipad + data
    std::vector<std::uint8_t> buf(BlockSize + dataLen);
    std::memcpy(buf.data(), ipad, BlockSize);
    std::memcpy(buf.data() + BlockSize, data, dataLen);
    hashFunc(buf.data(), buf.size(), inner);

    // outer = H(opad || inner)
    std::uint8_t outerInput[BlockSize + DigestSize];
    std::memcpy(outerInput, opad, BlockSize);
    std::memcpy(outerInput + BlockSize, inner, DigestSize);
    hashFunc(outerInput, sizeof(outerInput), out);
}

void hmacSha1(const void* key, std::size_t keyLen,
              const void* data, std::size_t dataLen,
              std::uint8_t out[20]) {
    hmacGeneric<64, 20>(key, keyLen, data, dataLen, out, sha1);
}

std::string hmacSha1(const std::string& key, const std::string& data) {
    std::uint8_t out[20];
    hmacSha1(key.data(), key.size(), data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 20);
}

void hmacSha256(const void* key, std::size_t keyLen,
                const void* data, std::size_t dataLen,
                std::uint8_t out[32]) {
    hmacGeneric<64, 32>(key, keyLen, data, dataLen, out, sha256);
}

std::string hmacSha256(const std::string& key, const std::string& data) {
    std::uint8_t out[32];
    hmacSha256(key.data(), key.size(), data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 32);
}

void hmacSha384(const void* key, std::size_t keyLen,
                const void* data, std::size_t dataLen,
                std::uint8_t out[48]) {
    // SHA-384 与 SHA-512 共用块大小（128 字节）
    hmacGeneric<128, 48>(key, keyLen, data, dataLen, out, sha384);
}

std::string hmacSha384(const std::string& key, const std::string& data) {
    std::uint8_t out[48];
    hmacSha384(key.data(), key.size(), data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 48);
}

void hmacSha512(const void* key, std::size_t keyLen,
                const void* data, std::size_t dataLen,
                std::uint8_t out[64]) {
    hmacGeneric<128, 64>(key, keyLen, data, dataLen, out, sha512);
}

std::string hmacSha512(const std::string& key, const std::string& data) {
    std::uint8_t out[64];
    hmacSha512(key.data(), key.size(), data.data(), data.size(), out);
    return std::string(reinterpret_cast<const char*>(out), 64);
}

} // namespace internal
} // namespace crypto
} // namespace office
} // namespace zq
