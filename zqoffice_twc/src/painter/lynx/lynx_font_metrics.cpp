// =============================================================================
// src/painter/lynx/lynx_font_metrics.cpp
//
// LynxFontMetrics 实现
// =============================================================================
#include "painter/lynx/lynx_font_metrics.h"

namespace zq {
namespace office {
namespace painter {
namespace lynx {

LynxFontMetrics::LynxFontMetrics()
    : has_underline_position(false)
    , has_underline_thickness(false)
    , underline_position(0.0f)
    , underline_thickness(0.0f)
    , has_strikeout_position(false)
    , has_strikeout_thickness(false)
    , strikeout_position(0.0f)
    , strikeout_thickness(0.0f) {}

bool LynxFontMetrics::hasUnderlinePosition(float* out) const {
    if (has_underline_position && out) {
        *out = underline_position;
        return true;
    }
    return has_underline_position;
}

bool LynxFontMetrics::hasUnderlineThickness(float* out) const {
    if (has_underline_thickness && out) {
        *out = underline_thickness;
        return true;
    }
    return has_underline_thickness;
}

bool LynxFontMetrics::hasStrikeoutPosition(float* out) const {
    if (has_strikeout_position && out) {
        *out = strikeout_position;
        return true;
    }
    return has_strikeout_position;
}

bool LynxFontMetrics::hasStrikeoutThickness(float* out) const {
    if (has_strikeout_thickness && out) {
        *out = strikeout_thickness;
        return true;
    }
    return has_strikeout_thickness;
}

}  // namespace lynx
}  // namespace painter
}  // namespace office
}  // namespace zq
