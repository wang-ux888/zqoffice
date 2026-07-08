// =============================================================================
// src/drawing/i_office_art_client_data.h
//
// IOfficeArtClientData 接口
//
//   客户端数据接口，由 PPT/Excel 等具体客户端实现，用于从 Escher ClientData
//   记录中解析出客户端特定的数据（如 PPT placeholder 信息）。
//
//   对应 [MS-ODRAW] 2.4.8 OfficeArtClientData 记录（recType=0xF00A）
//
// 导出符号对齐：
//   - ??0IOfficeArtClientData (构造/拷贝)
//   - ??1...UEAA@XZ (virtual ~)
//   - ??4... (operator=)
//   - ??_7... (vtable)
//   - ?IsPlaceholder@...QEBA_NXZ (non-virtual bool IsPlaceholder() const)
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace zq {
namespace office {
namespace drawing {

/// OfficeArt 客户端数据接口
///
/// 由具体客户端实现，EscherDrawingConvert::ConvertClientDataRecord 通过此接口
/// 回调客户端解析逻辑。
class IOfficeArtClientData {
public:
    virtual ~IOfficeArtClientData() = default;

    /// 记录 ID（Escher recType）
    virtual std::uint16_t getRecordId() const = 0;

    /// 记录名称
    virtual std::string getRecordName() const = 0;

    /// 记录数据大小（不含头部）
    virtual int getRecordSize() = 0;

    /// 解析记录数据
    /// @param data 记录数据指针（不含头部）
    /// @param size 数据字节数
    /// @return 实际消费的字节数，<0 表示错误
    virtual int fillFields(const unsigned char* data, int size) = 0;

    /// 是否为占位符
    /// 派生类（如 PPT 的 OfficeArtClientData）可以覆盖此行为
    bool IsPlaceholder() const { return isPlaceholder_; }

protected:
    // 派生类通过此字段控制 IsPlaceholder 行为
    bool isPlaceholder_ = false;
};

} // namespace drawing
} // namespace office
} // namespace zq
