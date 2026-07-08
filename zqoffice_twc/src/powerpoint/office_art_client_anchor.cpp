// =============================================================================
// src/powerpoint/office_art_client_anchor.cpp
//
// PowerPoint 客户端锚点记录实现
//
//   PPT 锚点格式（8 字节）：
//     Offset 0: top    (int16, 1/8 point)
//     Offset 2: left   (int16, 1/8 point)
//     Offset 4: right  (int16, 1/8 point)
//     Offset 6: bottom (int16, 1/8 point)
//
//   fillFields 直接解析 8 字节小端 int16 至成员变量，
//   不依赖基类 Read（避免基类视图指针语义导致拷贝/移动问题）。
// =============================================================================
#include "office_art_client_anchor.h"

#include <cstdint>

namespace zq {
namespace office {
namespace powerpoint {

namespace {

/// 读取小端 16 位有符号整数
inline std::int16_t readI16LE(const unsigned char* p) {
    return static_cast<std::int16_t>(
        static_cast<std::uint16_t>(p[0]) |
        (static_cast<std::uint16_t>(p[1]) << 8));
}

} // namespace

// ---------------------------------------------------------------------------
// 虚函数实现
// ---------------------------------------------------------------------------

int OfficeArtClientAnchor::getRecordSize() {
    // PPT 锚点固定 8 字节（4×int16）
    return static_cast<int>(kPPTAnchorSize);
}

int OfficeArtClientAnchor::fillFields(const unsigned char* data, int size) {
    if (!data || size < 0) {
        return -1;
    }

    // PPT 锚点需要至少 8 字节（4×int16）
    if (size < static_cast<int>(kPPTAnchorSize)) {
        return -1;
    }

    // 直接解析 4×int16（小端）
    // 字段顺序（与 [MS-PPT] OfficeArtClientAnchorRecord 一致）：
    //   top / left / right / bottom
    top_    = readI16LE(data + 0);
    left_   = readI16LE(data + 2);
    right_  = readI16LE(data + 4);
    bottom_ = readI16LE(data + 6);

    // 返回消费字节数（8 字节 PPT 锚点）
    return static_cast<int>(kPPTAnchorSize);
}

} // namespace powerpoint
} // namespace office
} // namespace zq
