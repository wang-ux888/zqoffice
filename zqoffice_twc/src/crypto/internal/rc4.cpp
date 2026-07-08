// =============================================================================
// src/crypto/internal/rc4.cpp
//
// RC4 流密码实现（RFC 6229）
// =============================================================================
#include "rc4.h"

#include <cstring>

namespace zq {
namespace office {
namespace crypto {
namespace internal {

void rc4Init(RC4State& state, const void* key, std::size_t keyLen) {
    // KSA：初始化 S 盒为 0-255，然后按密钥置换
    for (int i = 0; i < 256; ++i) {
        state.s[i] = static_cast<std::uint8_t>(i);
    }
    state.i = 0;
    state.j = 0;

    // 使用 int 类型作为中间变量，避免 uint8_t 算术的任何潜在截断/提升问题
    const auto* k = static_cast<const std::uint8_t*>(key);
    int j = 0;
    for (int i = 0; i < 256; ++i) {
        j = (j + state.s[i] + k[i % keyLen]) & 0xFF;
        // 交换 s[i] 和 s[j]
        std::uint8_t tmp = state.s[i];
        state.s[i] = state.s[j];
        state.s[j] = tmp;
    }
}

void rc4Process(RC4State& state, void* data, std::size_t len) {
    auto* p = static_cast<std::uint8_t*>(data);
    // 用 int 缓存 i/j 避免每步截断；循环结束后再写回 state
    int i = state.i;
    int j = state.j;
    for (std::size_t n = 0; n < len; ++n) {
        i = (i + 1) & 0xFF;
        j = (j + state.s[i]) & 0xFF;
        // 交换 s[i] 和 s[j]
        std::uint8_t tmp = state.s[i];
        state.s[i] = state.s[j];
        state.s[j] = tmp;
        // 生成密钥流字节并 XOR
        std::uint8_t k = state.s[(state.s[i] + state.s[j]) & 0xFF];
        p[n] ^= k;
    }
    state.i = static_cast<std::uint8_t>(i);
    state.j = static_cast<std::uint8_t>(j);
}

} // namespace internal
} // namespace crypto
} // namespace office
} // namespace zq
