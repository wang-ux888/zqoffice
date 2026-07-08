// =============================================================================
// src/excel/office_art_client_textbox.h
//
// Excel 客户端文本框记录（[MS-ODRAW] 2.4.9 OfficeArtClientTextbox）
//
//   继承自 EscherRecord + 实现 IOfficeArtClientTextbox。
//
//   Excel 文本框记录（recType=0xF010）包含 BIFF 格式的文本数据，
//   通过 ConvertToTextBody 转换为统一 TextBody 模型。
//
// 导出符号对齐：
//   - 构造（默认/移动）
//   - virtual ~OfficeArtClientTextbox()
//   - virtual RecordType getRecordId() const
//   - virtual std::string getRecordName() const
//   - virtual int getRecordSize()
//   - virtual int fillFields(const unsigned char* data, int size)
//   - virtual unique_ptr<TextBody> ConvertToTextBody(IClient*)
//   - unique_ptr<TextBody> GetTextBody() / void SetTextBody(unique_ptr<TextBody>)
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "escher/escher_record.h"
#include "drawing/i_office_art_client_textbox.h"
#include "drawing/text_body.h"

namespace zq {
namespace office {
namespace excel {

/// Excel 客户端文本框记录
///
///   - 继承 EscherRecord：复用 Escher 记录解析（8 字节头 + 数据）
///   - 实现 IOfficeArtClientTextbox：供 EscherDrawingConvert 回调
///   - Excel 特有：从 BIFF 文本流中提取文本并转换为 TextBody
class OfficeArtClientTextbox : public escher::EscherRecord,
                                public drawing::IOfficeArtClientTextbox {
public:
    /// Escher ClientTextbox 记录类型（[MS-ODRAW] 2.4.9）
    static constexpr std::uint16_t kRecordType = 0xF010;

    OfficeArtClientTextbox() = default;

    /// 移动构造
    OfficeArtClientTextbox(OfficeArtClientTextbox&&) noexcept = default;

    /// 移动赋值
    OfficeArtClientTextbox& operator=(OfficeArtClientTextbox&&) noexcept = default;

    /// 禁止拷贝（EscherRecord 持有指针视图）
    OfficeArtClientTextbox(const OfficeArtClientTextbox&) = delete;
    OfficeArtClientTextbox& operator=(const OfficeArtClientTextbox&) = delete;

    ~OfficeArtClientTextbox() override = default;

    // -----------------------------------------------------------------------
    // IOfficeArtClientTextbox 接口实现
    // -----------------------------------------------------------------------

    /// 记录 ID（Escher recType = 0xF010）
    std::uint16_t getRecordId() const override { return kRecordType; }

    /// 记录名称
    std::string getRecordName() const override { return "OfficeArtClientTextbox"; }

    /// 记录数据大小（不含头部）
    int getRecordSize() override;

    /// 解析记录数据
    /// @param data 记录数据指针（不含 Escher 头部）
    /// @param size 数据字节数
    /// @return 实际消费的字节数，<0 表示错误
    int fillFields(const unsigned char* data, int size) override;

    /// 转换为 TextBody（Excel 特有：从 BIFF 文本流提取）
    /// @param client 客户端上下文（用于获取字体/样式等）
    /// @return 转换后的 TextBody，失败返回 nullptr
    std::unique_ptr<drawing::TextBody> convertToTextBody(
        drawing::IClient* client) override;

    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------

    /// 获取已解析的文本框（不转移所有权）
    std::unique_ptr<drawing::TextBody> GetTextBody();

    /// 设置文本框（转移所有权）
    void SetTextBody(std::unique_ptr<drawing::TextBody> body);

private:
    /// 已解析的文本框（可选，由 fillFields 解析或外部 SetTextBody 注入）
    std::unique_ptr<drawing::TextBody> textBody_;

    /// 原始记录数据副本（fillFields 时保存）
    std::vector<unsigned char> rawData_;
};

} // namespace excel
} // namespace office
} // namespace zq
