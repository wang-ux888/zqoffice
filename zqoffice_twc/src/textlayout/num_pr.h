// =============================================================================
// src/textlayout/num_pr.h
//
// NumPr 类（项目编号）
//
//   段落项目编号属性，含编号 ID + 级别。
//
//   对应 OOXML a:pPr/a:buChar/a:buAutoNum/a:buFont + w:pPr w:numPr：
//     - numId：编号方案 ID
//     - ilvl：编号级别（0-8）
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {
namespace textlayout {

/// 项目编号属性
class NumPr {
public:
    NumPr() = default;
    ~NumPr() = default;

    /// 编号方案 ID（numId）
    std::int32_t GetNumId() const { return numId_; }
    void SetNumId(std::int32_t id) { numId_ = id; }

    /// 编号级别（ilvl，0-8）
    std::int32_t GetIlvl() const { return ilvl_; }
    void SetIlvl(std::int32_t lvl) { ilvl_ = lvl; }

    /// 是否有编号
    bool HasNumbering() const { return numId_ >= 0; }

private:
    std::int32_t numId_ = -1;   // -1 表示无编号
    std::int32_t ilvl_ = 0;     // 0-8
};

} // namespace textlayout
} // namespace office
} // namespace zq
