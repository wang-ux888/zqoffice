// =============================================================================
// src/drawing/theme.cpp
//
// Theme 类实现
// =============================================================================
#include "theme.h"

namespace zq {
namespace office {
namespace drawing {

Theme::Theme() = default;
Theme::~Theme() = default;

void Theme::setColor(ThemeType type, const std::string& rgb) {
    if (type == ThemeType::kUnknown) return;
    colors_[type] = rgb;
}

std::string Theme::getColor(ThemeType type) const {
    auto it = colors_.find(type);
    if (it == colors_.end()) return "";
    return it->second;
}

bool Theme::hasColor(ThemeType type) const {
    return colors_.find(type) != colors_.end();
}

bool Theme::isValid() const {
    // 至少包含 12 个基础颜色方案中的 4 个
    int basicCount = 0;
    const ThemeType basics[] = {
        ThemeType::kDark1, ThemeType::kLight1,
        ThemeType::kDark2, ThemeType::kLight2,
        ThemeType::kAccent1, ThemeType::kAccent2,
        ThemeType::kAccent3, ThemeType::kAccent4,
        ThemeType::kAccent5, ThemeType::kAccent6,
        ThemeType::kHyperlink, ThemeType::kFollowedHyperlink,
    };
    for (auto t : basics) {
        if (hasColor(t)) ++basicCount;
    }
    return basicCount >= 4;
}

void Theme::clear() {
    name_.clear();
    colors_.clear();
    fontScheme_ = FontScheme{};
    fmtScheme_ = FmtScheme{};
}

} // namespace drawing
} // namespace office
} // namespace zq
