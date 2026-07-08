// =============================================================================
// src/textlayout/object_run.h
//
// ObjectRun 类（对象 Run）
//
//   对象 Run，表示文本流中的嵌入对象（如图片、形状占位、表格占位等）。
//   持有 RunDelegate 进行实际绘制，宽度/高度由 RunDelegate 决定。
//
//   与 TextRun 的差异：
//     - TextRun 持有文本 + RunMetrics，通过 IFontManager 测量
//     - ObjectRun 持有 RunDelegate，宽度/高度直接从委托获取
//
//   OBJECT_REPLACEMENT_CHARACTER：U+FFFC（对象替换字符），
//   表示文本流中该位置有一个对象占位（与 Unicode OBJ REPLACEMENT 语义一致）。
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "textlayout/base_run.h"
#include "textlayout/font_info.h"

namespace zq {
namespace office {
namespace textlayout {

class Paragraph;
class RunPr;
class RunDelegate;
class IFontManager;
class ICanvasHelper;

/// 对象 Run
class ObjectRun : public BaseRun {
public:
    /// 构造
    /// @param para 所属段落
    /// @param runPr Run 属性
    /// @param startCharPos 起始字符位置
    /// @param charCount 字符数（对象 Run 通常 charCount=1，对应 OBJECT_REPLACEMENT_CHARACTER）
    /// @param type Run 类型（kObject / kControlLine / kControlPage / kControlColumn / kControlTab）
    ObjectRun(Paragraph* para, RunPr* runPr,
              std::uint32_t startCharPos, std::uint32_t charCount,
              RunType type = RunType::kObject);

    ~ObjectRun() override = default;

    // -----------------------------------------------------------------------
    // BaseRun 纯虚方法实现
    // -----------------------------------------------------------------------

    /// 布局：从 RunDelegate 获取宽度/高度，更新 width_/height_
    void Layout(IFontManager* fontMgr) override;

    float GetAscent() const override;
    float GetDescent() const override;

    /// 对象 Run 无字符推进数组
    std::pair<std::uint32_t, const float*> GetAdvances() const override {
        return { 0, nullptr };
    }

    RunType GetType() const override { return type_; }

    /// 对象 Run 无 FontInfo
    const FontInfo* GetFontInfo() const override { return nullptr; }

    // -----------------------------------------------------------------------
    // 虚方法覆盖
    // -----------------------------------------------------------------------

    /// 控制符 Run（kControl*）算作控制 Run
    bool IsControlRun() const override;

    /// 控制符 Run 不可见
    bool Visible() const override;

    // -----------------------------------------------------------------------
    // ObjectRun 特有方法
    // -----------------------------------------------------------------------

    /// 获取 Run 委托
    RunDelegate* GetRunDelegate() const { return runDelegate_.get(); }

    /// 设置 Run 委托（转移所有权）
    void SetRunDelegate(std::unique_ptr<RunDelegate> delegate);

    /// 绘制对象到画布
    /// @param canvas 画布助手
    /// @param x X 坐标（px）
    /// @param y Y 坐标（px）
    void Draw(ICanvasHelper* canvas, float x, float y);

    // -----------------------------------------------------------------------
    // 静态常量
    // -----------------------------------------------------------------------

    /// U+FFFC 对象替换字符（UTF-8 编码：0xEF 0xBF 0xBC）
    static const std::string OBJECT_REPLACEMENT_CHARACTER;

private:
    std::unique_ptr<RunDelegate> runDelegate_;  // Run 委托（可选，控制 Run 可为空）
    RunType type_;                              // Run 类型
};

} // namespace textlayout
} // namespace office
} // namespace zq
