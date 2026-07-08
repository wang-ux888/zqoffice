// =============================================================================
// src/api/json/json_value.cpp
//
// JsonValue 拷贝/移动实现
// =============================================================================
#include "json_value.h"

namespace zq {
namespace office {
namespace json {

JsonValue::JsonValue(const JsonValue& other)
    : type_(other.type_)
    , bool_(other.bool_)
    , int_(other.int_)
    , double_(other.double_)
    , string_(other.string_)
    , array_(other.array_)
    , object_(other.object_) {
}

JsonValue::JsonValue(JsonValue&& other) noexcept
    : type_(other.type_)
    , bool_(other.bool_)
    , int_(other.int_)
    , double_(other.double_)
    , string_(std::move(other.string_))
    , array_(std::move(other.array_))
    , object_(std::move(other.object_)) {
    other.type_ = JsonType::Null;
}

JsonValue& JsonValue::operator=(const JsonValue& other) {
    if (this != &other) {
        type_ = other.type_;
        bool_ = other.bool_;
        int_ = other.int_;
        double_ = other.double_;
        string_ = other.string_;
        array_ = other.array_;
        object_ = other.object_;
    }
    return *this;
}

JsonValue& JsonValue::operator=(JsonValue&& other) noexcept {
    if (this != &other) {
        type_ = other.type_;
        bool_ = other.bool_;
        int_ = other.int_;
        double_ = other.double_;
        string_ = std::move(other.string_);
        array_ = std::move(other.array_);
        object_ = std::move(other.object_);
        other.type_ = JsonType::Null;
    }
    return *this;
}

} // namespace json
} // namespace office
} // namespace zq
