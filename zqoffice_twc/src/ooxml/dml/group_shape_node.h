// =============================================================================
// src/ooxml/dml/group_shape_node.h
//
// GroupShapeNode：DrawingML 形状树节点
//
//   DrawingML 中形状以树形结构组织：
//     - grpSp 可嵌套 grpSp，形成任意深度的树
//     - 每个节点有 spPr/grpSpPr 描述变换、几何、填充、轮廓
//     - 叶子节点通常为 sp/pic/cxnSp/graphicFrame
//   树结构用于支持 PowerPoint/Word/Excel 中的 group 形状
//
//   节点类型（GroupShapeNodeType）：
//     kGroup          - grpSp（可包含任意子节点）
//     kShape          - sp（叶子，描述形状）
//     kPicture        - pic（叶子，描述图片）
//     kConnector      - cxnSp（叶子，描述连接符）
//     kGraphicFrame   - graphicFrame（叶子，描述图表/表格）
//     kAbsoluteAnchor - xdr:absoluteAnchor（Excel drawing 顶层 anchor）
//     kOneCellAnchor  - xdr:oneCellAnchor（Excel drawing 顶层 anchor）
//     kTwoCellAnchor  - xdr:twoCellAnchor（Excel drawing 顶层 anchor）
//     kSlideTree      - p:spTree（PowerPoint slide 形状树根）
// =============================================================================
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../xml_reader.h"
#include "dml_common.h"

namespace zq {
namespace office {
namespace ooxml {
namespace dml {

/// GroupShapeNode 树节点
///
/// 设计：
///   - 树结构通过 std::unique_ptr 管理子节点所有权
///   - XmlNode 是视图，引用 XmlReader 拥有的文档（外部保活）
///   - 类型/属性/子节点是解析后的结构化数据，便于上层访问
class GroupShapeNode {
public:
    using Ptr = std::unique_ptr<GroupShapeNode>;

    GroupShapeNode() = default;
    explicit GroupShapeNode(GroupShapeNodeType type);

    // -----------------------------------------------------------------------
    // 基本信息
    // -----------------------------------------------------------------------

    /// 节点类型
    GroupShapeNodeType type() const { return type_; }
    void setType(GroupShapeNodeType t) { type_ = t; }

    /// 形状名称（nvSpPr/cNvPr/@name）
    const std::string& name() const { return name_; }
    void setName(const std::string& n) { name_ = n; }

    /// 形状 ID（nvSpPr/cNvPr/@id）
    const std::string& id() const { return id_; }
    void setId(const std::string& i) { id_ = i; }

    // -----------------------------------------------------------------------
    // 形状属性
    // -----------------------------------------------------------------------

    const ShapeProperties& properties() const { return props_; }
    ShapeProperties& properties() { return props_; }

    // -----------------------------------------------------------------------
    // 子节点
    // -----------------------------------------------------------------------

    const std::vector<Ptr>& children() const { return children_; }
    std::vector<Ptr>& children() { return children_; }

    /// 添加子节点（取得所有权）
    void addChild(Ptr child);

    /// 释放子节点所有权（用于转移）
    Ptr releaseChild(std::size_t index);

    /// 是否为组节点
    bool isGroup() const { return type_ == GroupShapeNodeType::kGroup; }

    /// 是否为叶子节点
    bool isLeaf() const {
        return type_ == GroupShapeNodeType::kShape ||
               type_ == GroupShapeNodeType::kPicture ||
               type_ == GroupShapeNodeType::kConnector ||
               type_ == GroupShapeNodeType::kGraphicFrame;
    }

    /// 直接子节点数量
    std::size_t childCount() const { return children_.size(); }

    /// 后代节点总数（递归，不含自身）
    std::size_t descendantCount() const;

    /// 查找指定类型的所有节点（递归，包括自身）
    /// @return 节点指针（不转移所有权，节点生命周期由树拥有）
    std::vector<GroupShapeNode*> findByType(GroupShapeNodeType t);

    // -----------------------------------------------------------------------
    // 原始 XML 节点（保留以便上层访问未解析字段）
    // -----------------------------------------------------------------------

    const XmlNode& xmlNode() const { return xmlNode_; }
    void setXmlNode(const XmlNode& n) { xmlNode_ = n; }

private:
    GroupShapeNodeType type_ = GroupShapeNodeType::kUnknown;
    std::string id_;
    std::string name_;
    ShapeProperties props_;
    std::vector<Ptr> children_;
    XmlNode xmlNode_;  // 视图，不拥有数据

    void findByTypeHelper_(GroupShapeNodeType t,
                           std::vector<GroupShapeNode*>& out);
};

} // namespace dml
} // namespace ooxml
} // namespace office
} // namespace zq
