// =============================================================================
// src/textlayout/tabs.h
//
// Tabs 类（制表符）
//
//   段落制表符属性，含自定义制表位列表 + 默认制表位宽度。
//
//   对应 OOXML a:pPr/a:tabLst + w:tabs/w:tab：
//     - 自定义制表位列表（位置 + 对齐方式 + 引导符）
//     - 默认制表位宽度（w:pPr w:tabs w:tab w:val="clear" pos=0）
// =============================================================================
#pragma once

#include <cstdint>
#include <vector>

namespace zq {
namespace office {
namespace textlayout {

/// 制表位对齐方式
enum class TabAlign : std::uint8_t {
    kUnknown = 0,
    kLeft,      // left   - 左对齐（默认）
    kCenter,    // center - 居中
    kRight,     // right  - 右对齐
    kDecimal,   // decimal- 小数点对齐
};

/// 制表位引导符
enum class TabLeader : std::uint8_t {
    kUnknown = 0,
    kNone,      // none   - 无
    kDot,       // dot    - 点
    kHyphen,    // hyphen - 连字符
    kUnderscore,// underscore - 下划线
};

/// 单个制表位
struct TabStop {
    float pos = 0.0f;               // 位置（px）
    TabAlign align = TabAlign::kLeft;   // 对齐方式
    TabLeader leader = TabLeader::kNone;// 引导符
};

/// 制表符集合
class Tabs {
public:
    Tabs() = default;
    ~Tabs() = default;

    /// 添加自定义制表位
    void AddTab(const TabStop& tab) { tabs_.push_back(tab); }

    /// 获取所有制表位
    const std::vector<TabStop>& GetTabs() const { return tabs_; }
    std::vector<TabStop>& GetTabs() { return tabs_; }

    /// 制表位数量
    std::size_t GetTabCount() const { return tabs_.size(); }

    /// 获取指定制表位
    const TabStop& GetTab(std::size_t i) const { return tabs_[i]; }

    /// 清空制表位
    void Clear() { tabs_.clear(); }

    /// 默认制表位宽度（px）
    float GetDefaultTabWidth() const { return defaultTabWidth_; }
    void SetDefaultTabWidth(float w) { defaultTabWidth_ = w; }

private:
    std::vector<TabStop> tabs_;
    float defaultTabWidth_ = 36.0f;  // 默认 0.5 英寸（36 px @ 72 dpi）
};

} // namespace textlayout
} // namespace office
} // namespace zq
