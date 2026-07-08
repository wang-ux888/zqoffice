// =============================================================================
// src/textlayout/indent.h
//
// Indent 类（缩进）
//
//   段落缩进属性，含 4 对（px + chars）共 8 个字段：
//     - Start / End / FirstLine / Hanging（px 单位）
//     - StartChars / EndChars / FirstLineChars / HangingChars（字符单位）
// =============================================================================
#pragma once

#include <cstdint>

namespace zq {
namespace office {
namespace textlayout {

/// 段落缩进
class Indent {
public:
    Indent() = default;
    ~Indent() = default;

    // -----------------------------------------------------------------------
    // px 单位
    // -----------------------------------------------------------------------

    float GetStart() const { return start_; }
    void SetStart(float v) { start_ = v; }

    float GetEnd() const { return end_; }
    void SetEnd(float v) { end_ = v; }

    float GetFirstLine() const { return firstLine_; }
    void SetFirstLine(float v) { firstLine_ = v; }

    float GetHanging() const { return hanging_; }
    void SetHanging(float v) { hanging_ = v; }

    // -----------------------------------------------------------------------
    // 字符单位
    // -----------------------------------------------------------------------

    int GetStartChars() const { return startChars_; }
    void SetStartChars(int v) { startChars_ = v; }

    int GetEndChars() const { return endChars_; }
    void SetEndChars(int v) { endChars_ = v; }

    int GetFirstLineChars() const { return firstLineChars_; }
    void SetFirstLineChars(int v) { firstLineChars_ = v; }

    int GetHangingChars() const { return hangingChars_; }
    void SetHangingChars(int v) { hangingChars_ = v; }

private:
    // px 单位
    float start_ = 0.0f;
    float end_ = 0.0f;
    float firstLine_ = 0.0f;
    float hanging_ = 0.0f;

    // 字符单位
    int startChars_ = 0;
    int endChars_ = 0;
    int firstLineChars_ = 0;
    int hangingChars_ = 0;
};

} // namespace textlayout
} // namespace office
} // namespace zq
