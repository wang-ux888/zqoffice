// =============================================================================
// src/excel/office_art_client_textbox.cpp
//
// Excel 客户端文本框记录实现
//
//   Excel 文本框记录的原始数据是 BIFF 文本流（含 TxORecord 等），
//   完整解析需 BIFF 记录流解析器支持（Phase E 之后引入）。
//   本阶段实现：
//     - fillFields：保存原始数据，返回消费字节数
//     - convertToTextBody：返回已缓存的 textBody_（若已通过 SetTextBody 注入）
//     - GetTextBody/SetTextBody：所有权转移
// =============================================================================
#include "office_art_client_textbox.h"

#include <algorithm>

namespace zq {
namespace office {
namespace excel {

// ---------------------------------------------------------------------------
// IOfficeArtClientTextbox 接口实现
// ---------------------------------------------------------------------------

int OfficeArtClientTextbox::getRecordSize() {
    // 返回原始数据大小（不含 Escher 头部）
    return static_cast<int>(rawData_.size());
}

int OfficeArtClientTextbox::fillFields(const unsigned char* data, int size) {
    if (size < 0) {
        return -1;
    }

    // size=0 时 data 可为 nullptr，视为空数据合法
    if (size == 0) {
        rawData_.clear();
        return 0;
    }

    if (!data) {
        return -1;
    }

    // 保存原始数据副本（Escher 视图语义，需自行持有数据以便后续解析）
    rawData_.assign(data, data + size);

    // Excel 文本框完整解析需 BIFF 记录流支持（Phase E 引入），
    // 当前阶段仅保存原始数据，返回消费字节数。
    return size;
}

std::unique_ptr<drawing::TextBody> OfficeArtClientTextbox::convertToTextBody(
    drawing::IClient* /*client*/) {
    // 优先返回已缓存的 textBody_（由 SetTextBody 注入或前次解析结果）
    if (textBody_) {
        return std::move(textBody_);
    }

    // Excel BIFF 文本流完整解析需 Phase E（BIFF 记录流扩展）支持，
    // 当前阶段返回空 TextBody 占位，避免 nullptr 影响下游流程。
    // 后续 Phase 将在此处解析 BIFF TxORecord 并构建 TextBody。
    return std::make_unique<drawing::TextBody>();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

std::unique_ptr<drawing::TextBody> OfficeArtClientTextbox::GetTextBody() {
    return std::move(textBody_);
}

void OfficeArtClientTextbox::SetTextBody(std::unique_ptr<drawing::TextBody> body) {
    textBody_ = std::move(body);
}

} // namespace excel
} // namespace office
} // namespace zq
