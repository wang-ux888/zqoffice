// =============================================================================
// zq/office/types.h
//
// ZQOffice 公共类型定义
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

namespace zq {
namespace office {

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
enum class FileFormat : int {
    Unknown      = 0,
    Xlsx         = 1,  // OOXML Excel
    Xls          = 2,  // OLE2 BIFF Excel
    Csv          = 3,  // 纯文本 CSV
    Pptx         = 4,  // OOXML PowerPoint
    Ppt          = 5,  // OLE2 PowerPoint
    Docx         = 6,  // OOXML Word
    Doc          = 7,  // OLE2 Word
};

// ---------------------------------------------------------------------------
// 初始化模式（与 TTOfficeInitialize 的 mode 参数对齐）
//   0 = 完整模式（默认，加载 ICU 等可选依赖）
//   1 = 最小模式（仅核心解析，跳过 ICU）
//   2 = 调试模式（启用详细日志）
// ---------------------------------------------------------------------------
enum class InitMode : int {
    Full    = 0,
    Minimal = 1,
    Debug   = 2,
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
enum class ErrorCode : int {
    OK                  = 0,
    Unknown             = 1,
    InvalidArgument     = 2,
    FileNotFound        = 3,
    InvalidFormat       = 4,
    UnsupportedFormat   = 5,
    PasswordRequired    = 6,
    WrongPassword       = 7,
    CorruptedFile       = 8,
    OutOfMemory         = 9,
    InternalError       = 10,
    NotImplemented      = 11,
    EncodingError       = 12,
    XmlParseError       = 13,
    ZipError            = 14,
    Ole2Error           = 15,
    BiffError           = 16,
    EscherError         = 17,
    EncryptionError     = 18,
    ConversionError     = 19,
};

// ---------------------------------------------------------------------------
// ARGB 颜色（与 ColorHelper.SplitARGB 对齐）
// ---------------------------------------------------------------------------
struct Color {
    std::uint8_t a = 0xFF;
    std::uint8_t r = 0x00;
    std::uint8_t g = 0x00;
    std::uint8_t b = 0x00;

    constexpr Color() = default;
    constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 0xFF)
        : a(a), r(r), g(g), b(b) {}

    constexpr std::uint32_t toArgb() const {
        return (static_cast<std::uint32_t>(a) << 24)
             | (static_cast<std::uint32_t>(r) << 16)
             | (static_cast<std::uint32_t>(g) <<  8)
             | (static_cast<std::uint32_t>(b));
    }

    static constexpr Color fromArgb(std::uint32_t argb) {
        return Color(
            static_cast<std::uint8_t>((argb >> 16) & 0xFF),
            static_cast<std::uint8_t>((argb >>  8) & 0xFF),
            static_cast<std::uint8_t>((argb      ) & 0xFF),
            static_cast<std::uint8_t>((argb >> 24) & 0xFF));
    }
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct Point {
    float x = 0.0f;
    float y = 0.0f;
    constexpr Point() = default;
    constexpr Point(float x, float y) : x(x), y(y) {}
};

struct TTPoint {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr TTPoint() = default;
    constexpr TTPoint(std::int32_t x, std::int32_t y) : x(x), y(y) {}
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
struct RRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float radiusX = 0.0f;
    float radiusY = 0.0f;
};

// ---------------------------------------------------------------------------
// 转换结果（所有 XxxToZQYyy / ZQYyyToXxx 返回的 JSON 字符串外层包装）
//   { "code": 0, "message": "ok", "data": { ... } }
// ---------------------------------------------------------------------------
struct ExportResult {
    ErrorCode code = ErrorCode::OK;
    std::string message;
    std::string data;  // JSON 字符串

    /// 序列化为对外 JSON 字符串
    std::string toJson() const;

    /// （与 Utils::TryParseExportResultErrorCode 对齐）
    static ErrorCode parseErrorCode(const char* s);
};

// ---------------------------------------------------------------------------
// 通用矩形（用于布局）
// ---------------------------------------------------------------------------
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    constexpr Rect() = default;
    constexpr Rect(float x, float y, float w, float h)
        : x(x), y(y), width(w), height(h) {}

    constexpr float right() const { return x + width; }
    constexpr float bottom() const { return y + height; }
    constexpr bool isEmpty() const { return width <= 0.0f || height <= 0.0f; }
};

// ---------------------------------------------------------------------------
// 文件格式扩展名集合（与 FormatSelector 静态常量对齐）
// 由 format_selector.cpp 提供
// ---------------------------------------------------------------------------
class FormatSelector;

} // namespace office
} // namespace zq
