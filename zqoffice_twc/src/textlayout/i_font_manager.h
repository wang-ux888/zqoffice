// =============================================================================
// src/textlayout/i_font_manager.h
//
// IFontManager 接口（字体管理器）
//
//   字体管理器抽象接口，由具体平台实现（如 FreeType / GDI / CoreText）。
//   BaseRun::Layout(IFontManager*) 通过此接口测量文本，获取字符推进数组、
//   ascent/descent 等度量数据。
//
//   本阶段仅定义接口，具体实现由后续 Phase（渲染层）提供。
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "textlayout/font_info.h"

namespace zq {
namespace office {
namespace textlayout {

class RunPr;

/// 字体管理器接口
class IFontManager {
public:
    virtual ~IFontManager() = default;

    /// 加载字体（返回字体 ID，0 表示失败）
    /// @param fontName 字体名
    /// @param fontSize 字号（pt）
    /// @return 字体 ID（>0 成功）
    virtual std::uint32_t LoadFont(const std::string& fontName, float fontSize) = 0;

    /// 测量文本（获取字符推进数组 + ascent/descent）
    /// @param fontId 字体 ID（LoadFont 返回值）
    /// @param text 文本（UTF-8）
    /// @param charCount 字符数（UTF-8 字节数）
    /// @param advances 输出：每字符推进宽度（px）
    /// @param ascent 输出：ascent
    /// @param descent 输出：descent
    /// @return 成功返回 true
    virtual bool MeasureText(std::uint32_t fontId,
                              const char* text, std::uint32_t charCount,
                              std::vector<float>& advances,
                              float& ascent, float& descent) = 0;

    /// 获取字体度量信息
    /// @param fontId 字体 ID
    /// @return FontInfo 指针，失败返回 nullptr
    virtual const FontInfo* GetFontInfo(std::uint32_t fontId) const = 0;

    /// 从 RunPr 加载字体
    /// @param runPr Run 属性（含字体名、字号等）
    /// @return 字体 ID
    virtual std::uint32_t LoadFontFromRunPr(const RunPr* runPr) = 0;
};

} // namespace textlayout
} // namespace office
} // namespace zq
