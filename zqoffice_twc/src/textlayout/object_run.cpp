// =============================================================================
// src/textlayout/object_run.cpp
//
// ObjectRun 类实现
// =============================================================================
#include "object_run.h"

#include "i_canvas_helper.h"
#include "i_font_manager.h"
#include "run_delegate.h"
#include "run_pr.h"

namespace zq {
namespace office {
namespace textlayout {

// U+FFFC OBJECT REPLACEMENT CHARACTER（UTF-8 编码：0xEF 0xBF 0xBC）
const std::string ObjectRun::OBJECT_REPLACEMENT_CHARACTER =
    std::string("\xEF\xBF\xBC");

ObjectRun::ObjectRun(Paragraph* para, RunPr* runPr,
                     std::uint32_t startCharPos, std::uint32_t charCount,
                     RunType type)
    : BaseRun(para, runPr, startCharPos, charCount)
    , type_(type) {}

void ObjectRun::Layout(IFontManager* /*fontMgr*/) {
    // 控制符 Run（kControl*）无尺寸
    if (IsControlRun()) {
        SetWidth(0.0f);
        SetHeight(0.0f);
        return;
    }

    // 对象 Run：从 RunDelegate 获取尺寸
    if (runDelegate_) {
        SetWidth(runDelegate_->GetWidth());
        SetHeight(runDelegate_->GetHeight());
    } else {
        SetWidth(0.0f);
        SetHeight(0.0f);
    }
}

float ObjectRun::GetAscent() const {
    // 对象 Run 的 ascent = 完整高度（基线在底部）
    return GetHeight();
}

float ObjectRun::GetDescent() const {
    // 对象 Run 无 descent
    return 0.0f;
}

bool ObjectRun::IsControlRun() const {
    return type_ == RunType::kControlLine ||
           type_ == RunType::kControlPage ||
           type_ == RunType::kControlColumn ||
           type_ == RunType::kControlTab;
}

bool ObjectRun::Visible() const {
    // 控制符 Run 不可见
    return !IsControlRun();
}

void ObjectRun::SetRunDelegate(std::unique_ptr<RunDelegate> delegate) {
    runDelegate_ = std::move(delegate);
}

void ObjectRun::Draw(ICanvasHelper* canvas, float x, float y) {
    if (!canvas || !runDelegate_) {
        return;
    }
    runDelegate_->Draw(canvas, x, y, GetWidth(), GetHeight());
}

} // namespace textlayout
} // namespace office
} // namespace zq
