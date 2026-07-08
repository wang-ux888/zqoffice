// =============================================================================
// src/drawing/i_office_art_client_textbox.h
//
// IOfficeArtClientTextbox 接口
//
//   客户端文本框接口，由 Excel/PPT 等具体客户端实现，用于从 Escher 记录中
//   解析出文本数据并转换为 TextBody。
//
//   对应 [MS-ODRAW] 2.4.9 OfficeArtClientTextbox 记录（recType=0xF010 附近）
//   实际记录由客户端定义，本接口提供统一访问入口。
//
// 导出符号对齐：
//   - ??0IOfficeArtClientTextbox (构造/拷贝)
//   - ??1...UEAA@XZ (virtual ~)
//   - ??4... (operator=)
//   - ??_7... (vtable)
//   - ?getRecordName@...UEAA...XZ (virtual getRecordName)
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace zq {
namespace office {
namespace drawing {

class TextBody;
class IClient;

/// OfficeArt 客户端文本框接口
///
/// 由具体客户端（如 excel::OfficeArtClientTextbox）实现，
/// EscherDrawingConvert::ConvertClientTextbox 通过此接口回调客户端解析逻辑。
class IOfficeArtClientTextbox {
public:
    virtual ~IOfficeArtClientTextbox() = default;

    /// 记录 ID（Escher recType）
    virtual std::uint16_t getRecordId() const = 0;

    /// 记录名称（用于日志/调试）
    virtual std::string getRecordName() const = 0;

    /// 记录数据大小（不含头部）
    virtual int getRecordSize() = 0;

    /// 解析记录数据
    /// @param data 记录数据指针（不含头部）
    /// @param size 数据字节数
    /// @return 实际消费的字节数，<0 表示错误
    virtual int fillFields(const unsigned char* data, int size) = 0;

    /// 转换为 TextBody
    /// @param client 客户端上下文（用于获取字体/样式等）
    /// @return 转换后的 TextBody，失败返回 nullptr
    virtual std::unique_ptr<TextBody> convertToTextBody(IClient* client) = 0;
};

} // namespace drawing
} // namespace office
} // namespace zq
