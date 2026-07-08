// =============================================================================
// src/textlayout/p_pr.cpp
//
// PPr 类实现
// =============================================================================
#include "p_pr.h"
#include "run_pr.h"

namespace zq {
namespace office {
namespace textlayout {

PPr::PPr() = default;
PPr::~PPr() = default;

void PPr::SetDefaultRunPr(std::unique_ptr<RunPr> r) {
    defaultRunPr_ = std::move(r);
}

} // namespace textlayout
} // namespace office
} // namespace zq
