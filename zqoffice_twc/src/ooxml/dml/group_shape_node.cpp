// =============================================================================
// src/ooxml/dml/group_shape_node.cpp
// =============================================================================
#include "group_shape_node.h"

namespace zq {
namespace office {
namespace ooxml {
namespace dml {

GroupShapeNode::GroupShapeNode(GroupShapeNodeType type) : type_(type) {}

void GroupShapeNode::addChild(Ptr child) {
    if (child) {
        children_.push_back(std::move(child));
    }
}

GroupShapeNode::Ptr GroupShapeNode::releaseChild(std::size_t index) {
    if (index >= children_.size()) return nullptr;
    Ptr released = std::move(children_[index]);
    children_.erase(children_.begin() + index);
    return released;
}

std::size_t GroupShapeNode::descendantCount() const {
    std::size_t total = 0;
    for (const auto& c : children_) {
        if (c) {
            total += 1 + c->descendantCount();
        }
    }
    return total;
}

std::vector<GroupShapeNode*> GroupShapeNode::findByType(GroupShapeNodeType t) {
    std::vector<GroupShapeNode*> result;
    findByTypeHelper_(t, result);
    return result;
}

void GroupShapeNode::findByTypeHelper_(GroupShapeNodeType t,
                                        std::vector<GroupShapeNode*>& out) {
    if (type_ == t) {
        out.push_back(this);
    }
    for (auto& c : children_) {
        if (c) {
            c->findByTypeHelper_(t, out);
        }
    }
}

} // namespace dml
} // namespace ooxml
} // namespace office
} // namespace zq
