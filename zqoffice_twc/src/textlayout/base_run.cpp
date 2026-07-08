// =============================================================================
// src/textlayout/base_run.cpp
//
// BaseRun 类实现
// =============================================================================
#include "base_run.h"
#include "run_pr.h"

namespace zq {
namespace office {
namespace textlayout {

// 前向声明：Paragraph 在 p_pr.h 之外，需要单独的头文件
// 此处仅持有指针，不需要完整定义
class Paragraph;

BaseRun::BaseRun(Paragraph* para, RunPr* runPr,
                 std::uint32_t startCharPos, std::uint32_t charCount)
    : paragraph_(para)
    , runPr_(runPr)
    , startCharPos_(startCharPos)
    , charCount_(charCount) {}

} // namespace textlayout
} // namespace office
} // namespace zq
