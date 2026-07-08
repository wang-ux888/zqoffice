// =============================================================================
// src/textlayout/paragraph.h
//
// Paragraph 类（段落）
//
//   段落是文本布局的输入单元，持有一组 BaseRun（TextRun/ObjectRun/ControlRun）。
//
//   职责：
//     - 管理 Run 列表（AddTextRun/AddShapeRun/AddBreakRun）
//     - 管理段落属性（PPr：对齐/缩进/间距/编号/制表符/默认 RunPr）
//     - 字符位置 ↔ Run 索引 / UTF-8 位置互转
//     - 提供可断点查询（NextBreakablePos）供布局引擎使用
//     - 格式化 Run 列表（FormatRunList：合并相邻同属性 Run、设置字符位置）
//
//   生命周期：
//     1. 构造空段落
//     2. SetPPr(unique_ptr<PPr>) - 设置段落属性
//     3. AddTextRun/AddShapeRun/AddBreakRun - 添加 Run
//     4. FormatRunList() - 格式化（设置 startCharPos/charCount）
//     5. TextLayout::Layout(this, page) - 布局引擎消费
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "textlayout/textlayout_enums.h"

namespace zq {
namespace office {
namespace textlayout {

class PPr;
class RunPr;
class BaseRun;
class RunDelegate;
class Indent;
class LayoutPosition;

/// 段落
class Paragraph {
public:
    Paragraph();
    virtual ~Paragraph();

    Paragraph(const Paragraph&) = delete;
    Paragraph& operator=(const Paragraph&) = delete;
    Paragraph(Paragraph&&) noexcept = default;
    Paragraph& operator=(Paragraph&&) noexcept = default;

    // -----------------------------------------------------------------------
    // 段落属性
    // -----------------------------------------------------------------------

    /// 设置段落属性
    void SetPPr(std::unique_ptr<PPr> ppr);

    /// 取段落属性
    PPr* GetPPr() const { return pPr_.get(); }

    /// 取缩进
    Indent* GetIndent() const;

    // -----------------------------------------------------------------------
    // Run 管理
    // -----------------------------------------------------------------------

    /// 添加文本 Run
    /// @param runPr Run 属性（Paragraph 不接管所有权，仅持有指针）
    /// @param text 文本内容（UTF-8）
    /// @return 新添加的 Run（非拥有）
    const BaseRun* AddTextRun(RunPr* runPr, const std::string& text);

    /// 添加形状 Run（对象 Run）
    /// @param runPr Run 属性
    /// @param delegate Run 委托（转移所有权）
    /// @param isControl 是否控制符
    void AddShapeRun(RunPr* runPr, std::unique_ptr<RunDelegate> delegate,
                     bool isControl);

    /// 添加换行 Run
    /// @param breakType 换行类型（kLine/kPage/kColumn）
    void AddBreakRun(BreakType breakType);

    /// 启用 RunPr 唯一 ID（每个 Run 拥有独立的 RunPr 副本与唯一 ID）
    void EnableUniqueIDForEachRunPr();

    /// 格式化 Run 列表（设置 startCharPos/charCount，合并相邻同属性 Run）
    void FormatRunList();

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    /// 是否为空（无 Run 或无字符）
    bool Empty() const;

    /// 字符数
    std::uint32_t GetCharCount() const { return charCount_; }

    /// Run 数
    std::uint32_t GetRunCount() const {
        return static_cast<std::uint32_t>(runs_.size());
    }

    /// 取第 idx 个 Run
    BaseRun* GetRun(int idx) const;

    /// 按字符位置取 Run
    BaseRun* GetRunByCharPos(std::uint32_t charPos) const;

    /// 取段落文本内容（所有 TextRun 的文本拼接）
    const std::string& GetContent() const { return content_; }

    // -----------------------------------------------------------------------
    // 段落偏移
    // -----------------------------------------------------------------------

    std::uint32_t GetParagraphCharOffset() const { return paragraphCharOffset_; }
    void SetParagraphCharOffset(std::uint32_t offset) { paragraphCharOffset_ = offset; }

    // -----------------------------------------------------------------------
    // 位置转换
    // -----------------------------------------------------------------------

    /// 字符位置 → 布局位置
    LayoutPosition CharPosToLayoutPosition(std::uint32_t charPos) const;

    /// 字符位置 → UTF-8 字节位置
    std::uint32_t CharPosToUtf8Pos(std::uint32_t charPos) const;

    /// UTF-8 字节位置 → 字符位置
    std::uint32_t Utf8PosToCharPos(std::uint32_t utf8Pos) const;

    /// 布局位置 → 字符位置
    std::uint32_t LayoutPositionToCharPos(const LayoutPosition& pos) const;

    /// 取 Run 文本（按布局位置）
    std::string_view GetRunText(LayoutPosition& pos) const;

    /// 取 Run 文本（按字符位置与长度）
    std::string_view GetRunText(std::uint32_t charPos, std::uint32_t length) const;

    /// 下一可断点
    /// @param pos 当前位置
    /// @param outBreakType 输出：换行类型
    /// @return 下一可断点位置
    LayoutPosition NextBreakablePos(LayoutPosition pos,
                                     LineBreakType& outBreakType) const;

    /// 是否段落首字符
    bool IsFirstCharOfParagraph(std::uint32_t charPos) const;

private:
    // 添加文本内容（内部）
    std::uint32_t AddTextContent(const std::string& text);

    // 创建控制 Run（内部）
    std::unique_ptr<BaseRun> CreateControlRun(RunType type, RunPr* runPr,
                                               std::uint32_t startCharPos,
                                               bool isControl);

    // 生成新 Run（从已有 Run 切分）
    std::unique_ptr<BaseRun> GenerateNewRun(const BaseRun* src,
                                             std::uint32_t startChar,
                                             std::uint32_t charCount,
                                             bool isControl);

    // 转换换行类型
    LineBreakType ConvertBreakType(BreakType t) const;

    // 段落属性
    std::unique_ptr<PPr> pPr_;

    // Run 列表
    std::vector<std::unique_ptr<BaseRun>> runs_;

    // 文本内容（所有 TextRun 的拼接）
    std::string content_;

    // 字符数（按 UTF-8 字符数，非字节数）
    std::uint32_t charCount_ = 0;

    // 段落字符偏移（在文档内）
    std::uint32_t paragraphCharOffset_ = 0;

    // 是否启用 RunPr 唯一 ID
    bool uniqueIdEnabled_ = false;
    std::uint32_t nextUniqueId_ = 1;
};

} // namespace textlayout
} // namespace office
} // namespace zq
