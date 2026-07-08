// =============================================================================
// src/drawing/tt_shape_convert.h
//
// TTShapeConvert 类：形状转换器
//
//   从 OOXML（a:sp / a:pic / a:cxnSp）或二进制 OfficeArt 记录
//   转换为统一的 TTShape 模型。
// =============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "drawing/tt_shape.h"

namespace zq {
namespace office {

namespace ooxml {
class XmlNode;
}

namespace drawing {

class TTShapeConvert {
public:
    TTShapeConvert();
    ~TTShapeConvert();

    // -----------------------------------------------------------------------
    // OOXML 转换
    // -----------------------------------------------------------------------

    /// 从 OOXML a:sp 节点转换
    /// @param spNode  a:sp XML 节点
    /// @return 转换后的 TTShape，失败返回 nullptr
    std::shared_ptr<TTShape> convertFromOOXML(const ooxml::XmlNode& spNode);

    /// 从 OOXML a:pic 节点转换
    std::shared_ptr<TTShape> convertPicFromOOXML(const ooxml::XmlNode& picNode);

    /// 从 OOXML a:cxnSp 节点转换
    std::shared_ptr<TTShape> convertConnectorFromOOXML(
        const ooxml::XmlNode& cxnSpNode);

    // -----------------------------------------------------------------------
    // 二进制转换
    // -----------------------------------------------------------------------

    /// 从二进制 OfficeArt SpContainer 转换
    /// @param data  二进制数据
    /// @param size  数据字节数
    /// @return 转换后的 TTShape，失败返回 nullptr
    std::shared_ptr<TTShape> convertFromBinary(const unsigned char* data,
                                                std::size_t size);

    // -----------------------------------------------------------------------
    // 状态
    // -----------------------------------------------------------------------

    /// 错误信息
    const std::string& error() const { return error_; }

    /// 是否成功
    bool isSuccess() const { return success_; }

private:
    std::string error_;
    bool success_ = false;

    /// 解析 a:spPr 形状属性
    bool parseSpPr_(const ooxml::XmlNode& spPrNode, TTShape* shape);

    /// 解析 a:txBody 文本体
    bool parseTxBody_(const ooxml::XmlNode& txBodyNode, TTShape* shape);

    /// 提取 localName
    static std::string localName_(const std::string& fullName);
};

} // namespace drawing
} // namespace office
} // namespace zq
