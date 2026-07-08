// =============================================================================
// src/drawing/theme.h
//
// Theme 类：OOXML 主题模型
//
//   对应 OOXML ECMA-376 第 20.1.6 节 a:theme 元素，包含：
//     - a:clrScheme  : 颜色方案（12 个颜色槽位 dk1/lt1/dk2/lt2/accent1-6/hlink/folHlink）
//     - a:fontScheme : 字体方案（majorFont/minorFont + latin/ea/cs）
//     - a:fmtScheme  : 格式方案（fillStyleLst/lnStyleLst/effectStyleLst/bgFillStyleLst）
//
//   主题 XML 位置：
//     - PowerPoint: ppt/theme/theme1.xml
//     - Excel:      xl/theme/theme1.xml
//     - Word:       word/theme/theme1.xml
//
//   颜色值以 6 位十六进制 RRGGBB 字符串存储（无 # 前缀）
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "drawing/enums/theme_type.h"

namespace zq {
namespace office {
namespace drawing {

/// 主题字体方案
struct FontScheme {
    std::string majorLatin;          // a:majorFont/a:latin typeface=""
    std::string majorEastAsian;      // a:majorFont/a:ea typeface=""
    std::string majorComplexScript;  // a:majorFont/a:cs typeface=""
    std::string minorLatin;          // a:minorFont/a:latin typeface=""
    std::string minorEastAsian;      // a:minorFont/a:ea typeface=""
    std::string minorComplexScript;  // a:minorFont/a:cs typeface=""

    bool valid = false;
};

/// 主题格式方案（简化版，仅保留样式 ID）
/// 完整格式方案包含 fillStyleLst（3 个）、lnStyleLst（3 个）、
/// effectStyleLst（3 个）、bgFillStyleLst（3 个）的引用
struct FmtScheme {
    std::vector<int> fillStyleIds;      // 填充样式 ID 列表
    std::vector<int> lineStyleIds;      // 线条样式 ID 列表
    std::vector<int> effectStyleIds;    // 效果样式 ID 列表
    std::vector<int> bgFillStyleIds;    // 背景填充样式 ID 列表

    bool valid = false;
};

/// 主题模型
///
/// 包含颜色方案、字体方案、格式方案
class Theme {
public:
    Theme();
    ~Theme();

    // -----------------------------------------------------------------------
    // 主题元信息
    // -----------------------------------------------------------------------

    /// 主题名称（a:theme name="" 属性）
    void setName(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }

    // -----------------------------------------------------------------------
    // 颜色方案（a:clrScheme）
    // -----------------------------------------------------------------------

    /// 设置颜色槽位
    /// @param type  主题颜色类型（dk1/lt1/dk2/lt2/accent1-6/hlink/folHlink）
    /// @param rgb   6 位十六进制 RGB 字符串（如 "4472C4"）
    void setColor(ThemeType type, const std::string& rgb);

    /// 获取颜色槽位
    /// @param type  主题颜色类型
    /// @return RGB 字符串，未设置返回空字符串
    std::string getColor(ThemeType type) const;

    /// 是否包含指定颜色槽位
    bool hasColor(ThemeType type) const;

    /// 便捷访问 - 基础方案颜色
    std::string dark1() const { return getColor(ThemeType::kDark1); }
    std::string light1() const { return getColor(ThemeType::kLight1); }
    std::string dark2() const { return getColor(ThemeType::kDark2); }
    std::string light2() const { return getColor(ThemeType::kLight2); }
    std::string accent1() const { return getColor(ThemeType::kAccent1); }
    std::string accent2() const { return getColor(ThemeType::kAccent2); }
    std::string accent3() const { return getColor(ThemeType::kAccent3); }
    std::string accent4() const { return getColor(ThemeType::kAccent4); }
    std::string accent5() const { return getColor(ThemeType::kAccent5); }
    std::string accent6() const { return getColor(ThemeType::kAccent6); }
    std::string hyperlink() const { return getColor(ThemeType::kHyperlink); }
    std::string followedHyperlink() const {
        return getColor(ThemeType::kFollowedHyperlink);
    }

    /// 获取所有颜色槽位
    const std::unordered_map<ThemeType, std::string>& colors() const {
        return colors_;
    }

    // -----------------------------------------------------------------------
    // 字体方案（a:fontScheme）
    // -----------------------------------------------------------------------

    void setFontScheme(const FontScheme& scheme) { fontScheme_ = scheme; }
    const FontScheme& fontScheme() const { return fontScheme_; }
    FontScheme& fontScheme() { return fontScheme_; }

    // -----------------------------------------------------------------------
    // 格式方案（a:fmtScheme）
    // -----------------------------------------------------------------------

    void setFmtScheme(const FmtScheme& scheme) { fmtScheme_ = scheme; }
    const FmtScheme& fmtScheme() const { return fmtScheme_; }
    FmtScheme& fmtScheme() { return fmtScheme_; }

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------

    /// 是否有效（至少包含颜色方案）
    bool isValid() const;

    /// 清空所有数据
    void clear();

private:
    std::string name_;
    std::unordered_map<ThemeType, std::string> colors_;
    FontScheme fontScheme_;
    FmtScheme fmtScheme_;
};

} // namespace drawing
} // namespace office
} // namespace zq
