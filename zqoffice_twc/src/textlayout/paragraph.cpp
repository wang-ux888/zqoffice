// =============================================================================
// src/textlayout/paragraph.cpp
//
// Paragraph 类实现
// =============================================================================
#include "paragraph.h"

#include <algorithm>

#include "base_run.h"
#include "indent.h"
#include "layout_position.h"
#include "object_run.h"
#include "p_pr.h"
#include "run_delegate.h"
#include "run_pr.h"
#include "text_run.h"
#include "utils/u8string.h"

namespace zq {
namespace office {
namespace textlayout {

Paragraph::Paragraph() = default;
Paragraph::~Paragraph() = default;

void Paragraph::SetPPr(std::unique_ptr<PPr> ppr) {
    pPr_ = std::move(ppr);
}

Indent* Paragraph::GetIndent() const {
    return pPr_ ? pPr_->GetIndent() : nullptr;
}

const BaseRun* Paragraph::AddTextRun(RunPr* runPr, const std::string& text) {
    // 先添加文本内容，得到起始字符位置
    std::uint32_t startChar = charCount_;
    std::uint32_t added = AddTextContent(text);

    // 创建 TextRun
    auto run = std::make_unique<TextRun>(this, runPr, startChar, added, text);
    runs_.push_back(std::move(run));

    return runs_.back().get();
}

void Paragraph::AddShapeRun(RunPr* runPr,
                             std::unique_ptr<RunDelegate> delegate,
                             bool isControl) {
    // 对象 Run 占用 1 个字符（OBJECT_REPLACEMENT_CHARACTER）
    std::uint32_t startChar = charCount_;
    content_.append(ObjectRun::OBJECT_REPLACEMENT_CHARACTER);
    charCount_ += 1;

    // 创建 ObjectRun
    RunType type = isControl ? RunType::kObject : RunType::kObject;
    auto run = std::make_unique<ObjectRun>(this, runPr, startChar, 1, type);
    if (delegate) {
        run->SetRunDelegate(std::move(delegate));
    }
    runs_.push_back(std::move(run));
}

void Paragraph::AddBreakRun(BreakType breakType) {
    // 但为简化实现，这里仍占用 1 个字符位置以保持 charCount 一致性
    std::uint32_t startChar = charCount_;
    charCount_ += 1;

    // 根据换行类型选择 RunType
    RunType runType = RunType::kControlLine;
    switch (breakType) {
        case BreakType::kLine:   runType = RunType::kControlLine;   break;
        case BreakType::kPage:   runType = RunType::kControlPage;   break;
        case BreakType::kColumn: runType = RunType::kControlColumn; break;
        case BreakType::kUnknown:
        default:                 runType = RunType::kControlLine;   break;
    }

    auto run = std::make_unique<ObjectRun>(this, nullptr, startChar, 1, runType);
    runs_.push_back(std::move(run));
}

void Paragraph::EnableUniqueIDForEachRunPr() {
    uniqueIdEnabled_ = true;
    // 为所有现有 Run 的 RunPr 分配唯一 ID
    for (const auto& run : runs_) {
        RunPr* runPr = run->GetRunPr();
        if (runPr) {
            runPr->SetUniqueId(nextUniqueId_++);
        }
    }
}

void Paragraph::FormatRunList() {
    // 设置每个 Run 的 startCharPos（按 Run 顺序累加）
    std::uint32_t charPos = 0;
    for (const auto& run : runs_) {
        run->SetStartCharPos(charPos);
        charPos += run->GetCharCount();
    }
    charCount_ = charPos;
}

bool Paragraph::Empty() const {
    return runs_.empty() || charCount_ == 0;
}

BaseRun* Paragraph::GetRun(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(runs_.size())) {
        return nullptr;
    }
    return runs_[idx].get();
}

BaseRun* Paragraph::GetRunByCharPos(std::uint32_t charPos) const {
    for (const auto& run : runs_) {
        if (charPos >= run->GetStartCharPos() &&
            charPos < run->GetEndCharPos()) {
            return run.get();
        }
    }
    // 未找到时返回最后一个 Run（容错）
    if (!runs_.empty()) {
        return runs_.back().get();
    }
    return nullptr;
}

std::uint32_t Paragraph::AddTextContent(const std::string& text) {
    // 计算字符数（UTF-8 字符数，非字节数）
    std::uint32_t charCount = static_cast<std::uint32_t>(
        U8String::CalcCharCount(text.c_str(),
                                static_cast<int>(text.size())));
    content_.append(text);
    charCount_ += charCount;
    return charCount;
}

std::unique_ptr<BaseRun> Paragraph::CreateControlRun(RunType type,
                                                       RunPr* runPr,
                                                       std::uint32_t startCharPos,
                                                       bool isControl) {
    (void)isControl;  // 控制符 Run 始终为控制类型
    return std::make_unique<ObjectRun>(this, runPr, startCharPos, 1, type);
}

std::unique_ptr<BaseRun> Paragraph::GenerateNewRun(const BaseRun* src,
                                                     std::uint32_t startChar,
                                                     std::uint32_t charCount,
                                                     bool isControl) {
    (void)isControl;
    if (!src) return nullptr;

    // 从源 Run 切分出新 Run（仅支持 TextRun 切分）
    if (src->GetType() == RunType::kText) {
        const TextRun* textRun = static_cast<const TextRun*>(src);
        // 计算子文本
        std::uint32_t localStart = startChar - textRun->GetStartCharPos();
        std::string_view fullText = textRun->GetText();
        std::string_view subText = U8String::SubCharString(
            fullText, static_cast<int>(localStart), static_cast<int>(charCount));
        return std::make_unique<TextRun>(this, textRun->GetRunPr(),
                                          startChar, charCount,
                                          std::string(subText));
    }
    return nullptr;
}

LineBreakType Paragraph::ConvertBreakType(BreakType t) const {
    switch (t) {
        case BreakType::kLine:   return LineBreakType::kHardLine;
        case BreakType::kPage:   return LineBreakType::kPage;
        case BreakType::kColumn: return LineBreakType::kColumn;
        case BreakType::kUnknown:
        default:                 return LineBreakType::kNone;
    }
}

LayoutPosition Paragraph::CharPosToLayoutPosition(std::uint32_t charPos) const {
    // 查找包含 charPos 的 Run
    for (std::uint32_t i = 0; i < runs_.size(); ++i) {
        const auto& run = runs_[i];
        if (charPos >= run->GetStartCharPos() &&
            charPos < run->GetEndCharPos()) {
            return LayoutPosition(charPos, i);
        }
    }
    // 未找到时返回末尾位置
    return LayoutPosition(charCount_,
                           static_cast<std::uint32_t>(runs_.size()));
}

std::uint32_t Paragraph::CharPosToUtf8Pos(std::uint32_t charPos) const {
    // 字符位置 → UTF-8 字节位置
    return static_cast<std::uint32_t>(
        U8String::CharPosToByte(content_.c_str(),
                                static_cast<int>(content_.size()),
                                static_cast<int>(charPos)));
}

std::uint32_t Paragraph::Utf8PosToCharPos(std::uint32_t utf8Pos) const {
    // UTF-8 字节位置 → 字符位置
    return static_cast<std::uint32_t>(
        U8String::ByteToCharPos(content_.c_str(),
                                static_cast<int>(content_.size()),
                                static_cast<int>(utf8Pos)));
}

std::uint32_t Paragraph::LayoutPositionToCharPos(const LayoutPosition& pos) const {
    return pos.GetCharIdx();
}

std::string_view Paragraph::GetRunText(LayoutPosition& pos) const {
    if (pos.GetRunIdx() >= runs_.size()) {
        return {};
    }
    const auto& run = runs_[pos.GetRunIdx()];
    if (run->GetType() != RunType::kText) {
        return {};
    }
    const TextRun* textRun = static_cast<const TextRun*>(run.get());
    std::uint32_t localChar = pos.GetCharIdx() - run->GetStartCharPos();
    std::string_view fullText = textRun->GetText();
    std::string_view subText = U8String::SubCharString(
        fullText, static_cast<int>(localChar),
        static_cast<int>(run->GetCharCount() - localChar));
    // 推进 pos 到下一个 Run
    pos.NextRun();
    return subText;
}

std::string_view Paragraph::GetRunText(std::uint32_t charPos,
                                        std::uint32_t length) const {
    BaseRun* run = GetRunByCharPos(charPos);
    if (!run || run->GetType() != RunType::kText) {
        return {};
    }
    const TextRun* textRun = static_cast<const TextRun*>(run);
    std::uint32_t localChar = charPos - run->GetStartCharPos();
    std::string_view fullText = textRun->GetText();
    return U8String::SubCharString(
        fullText, static_cast<int>(localChar), static_cast<int>(length));
}

LayoutPosition Paragraph::NextBreakablePos(LayoutPosition pos,
                                            LineBreakType& outBreakType) const {
    outBreakType = LineBreakType::kNone;

    // 从 pos 开始扫描 Run，找到下一个可断点
    for (std::uint32_t i = pos.GetRunIdx(); i < runs_.size(); ++i) {
        const auto& run = runs_[i];

        // 控制符 Run（换行/分页/分栏）是天然断点
        if (run->IsControlRun()) {
            outBreakType = ConvertBreakType(
                run->GetType() == RunType::kControlPage ? BreakType::kPage :
                run->GetType() == RunType::kControlColumn ? BreakType::kColumn :
                BreakType::kLine);
            return LayoutPosition(run->GetStartCharPos(), i);
        }

        // TextRun 内部按空格断词
        if (run->GetType() == RunType::kText) {
            const TextRun* textRun = static_cast<const TextRun*>(run.get());
            const std::string& text = textRun->GetText();
            std::uint32_t startLocal = (i == pos.GetRunIdx()) ?
                (pos.GetCharIdx() - run->GetStartCharPos()) : 0;

            // 扫描空格
            std::uint32_t localPos = startLocal;
            std::string_view sv = text;
            while (localPos < run->GetCharCount()) {
                std::string_view ch = U8String::GetCharAt(sv, static_cast<int>(localPos));
                if (ch == " " || ch == "\t") {
                    // 找到空格，返回空格后的位置
                    outBreakType = LineBreakType::kSoftLine;
                    return LayoutPosition(run->GetStartCharPos() + localPos + 1, i);
                }
                ++localPos;
            }
        }
    }

    // 无可断点时返回末尾
    outBreakType = LineBreakType::kParagraphEnd;
    return LayoutPosition(charCount_,
                           static_cast<std::uint32_t>(runs_.size()));
}

bool Paragraph::IsFirstCharOfParagraph(std::uint32_t charPos) const {
    return charPos == 0;
}

} // namespace textlayout
} // namespace office
} // namespace zq
