// =============================================================================
// src/drawing/enums/bu_auto_num_type.h
//
// 项目符号自动编号类型枚举（BuAutoNumType）
//
//   对应 OOXML ECMA-376 第 20.1.10.5 节 ST_TextAutonumberScheme，
//   共 18 种自动编号方案，用于 a:buAutoNum type="" 属性。
//
//   示例：
//     alphaLcParenBoth  -> (a), (b), (c), ...
//     alphaUcPeriod     -> A., B., C., ...
//     arabicParenR      -> 1), 2), 3), ...
//     romanLcPeriod     -> i., ii., iii., ...
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace zq {
namespace office {
namespace drawing {

/// 项目符号自动编号类型
///
/// 与 OOXML ST_TextAutonumberScheme 对齐（ECMA-376 第 20.1.10.5 节）
/// 共 18 种方案
enum class BuAutoNumType : std::uint8_t {
    kUnknown = 0,

    // --- 字母编号 ---
    kAlphaLcParenBoth,    // (a) (b) (c) ... 小写字母+双括号
    kAlphaLcParenR,       // a) b) c) ...     小写字母+右括号
    kAlphaLcPeriod,       // a. b. c. ...     小写字母+点
    kAlphaUcParenBoth,    // (A) (B) (C) ... 大写字母+双括号
    kAlphaUcParenR,       // A) B) C) ...     大写字母+右括号
    kAlphaUcPeriod,       // A. B. C. ...     大写字母+点

    // --- 阿拉伯数字编号 ---
    kArabicParenBoth,     // (1) (2) (3) ...  数字+双括号
    kArabicParenR,        // 1) 2) 3) ...      数字+右括号
    kArabicPeriod,        // 1. 2. 3. ...      数字+点
    kArabicPlain,         // 1 2 3 ...         纯数字

    // --- 罗马数字编号 ---
    kRomanLcParenBoth,    // (i) (ii) (iii) ... 小写罗马+双括号
    kRomanLcParenR,       // i) ii) iii) ...     小写罗马+右括号
    kRomanLcPeriod,       // i. ii. iii. ...     小写罗马+点
    kRomanUcParenBoth,    // (I) (II) (III) ... 大写罗马+双括号
    kRomanUcParenR,       // I) II) III) ...     大写罗马+右括号
    kRomanUcPeriod,       // I. II. III. ...     大写罗马+点

    // --- 圆圈数字编号 ---
    kCircleLcDbdPlain,    // ① ② ③ ... 小写圆圈数字
    kCircleUcDbdPlain,    // 同上（大写形式，实际显示一致）
};

/// 枚举 → OOXML 字符串名称
const char* buAutoNumTypeToString(BuAutoNumType t);

/// 字符串 → 枚举
BuAutoNumType buAutoNumTypeFromString(std::string_view s);

} // namespace drawing
} // namespace office
} // namespace zq
