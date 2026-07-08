// =============================================================================
// src/api/json/json_value.h
//
// ZQOffice 内部 JSON 值类型
//
// 轻量级 JSON 实现（不依赖第三方库），用于 Phase H 转换器：
//   - XlsxConverter / PptxConverter / DocxConverter 输出 ZQ JSON
//   - 解析 ZQ JSON 输入（写入管线）
//
// 设计：
//   - 7 种类型：Null / Bool / Int / Double / String / Array / Object
//   - Int 使用 int64_t（避免 double 精度丢失）
//   - Array 使用 std::vector<JsonValue>
//   - Object 使用 std::vector<std::pair<std::string, JsonValue>>（保留插入顺序）
//   - 移动语义优先（避免大对象拷贝）
//
// 与 ZQOffice 命名空间约定一致：zq::office::json
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace zq {
namespace office {
namespace json {

/// JSON 值类型枚举
enum class JsonType {
    Null,
    Bool,
    Int,
    Double,
    String,
    Array,
    Object,
};

/// JSON 值类
class JsonValue {
public:
    /// 构造 Null
    JsonValue() : type_(JsonType::Null) {}

    /// 构造 Bool
    JsonValue(bool b) : type_(JsonType::Bool), bool_(b) {}

    /// 构造 Int（支持 int / int64_t）
    JsonValue(int v) : type_(JsonType::Int), int_(v) {}
    JsonValue(std::int64_t v) : type_(JsonType::Int), int_(v) {}

    /// 构造 Double
    JsonValue(double d) : type_(JsonType::Double), double_(d) {}

    /// 构造 String
    JsonValue(const char* s) : type_(JsonType::String), string_(s ? s : "") {}
    JsonValue(const std::string& s) : type_(JsonType::String), string_(s) {}
    JsonValue(std::string&& s) : type_(JsonType::String), string_(std::move(s)) {}

    /// 拷贝/移动
    JsonValue(const JsonValue& other);
    JsonValue(JsonValue&& other) noexcept;
    JsonValue& operator=(const JsonValue& other);
    JsonValue& operator=(JsonValue&& other) noexcept;
    ~JsonValue() = default;

    // -----------------------------------------------------------------------
    // 工厂方法
    // -----------------------------------------------------------------------

    /// 创建 Array
    static JsonValue array() {
        JsonValue v;
        v.type_ = JsonType::Array;
        v.array_.clear();
        return v;
    }

    /// 创建 Object
    static JsonValue object() {
        JsonValue v;
        v.type_ = JsonType::Object;
        v.object_.clear();
        return v;
    }

    // -----------------------------------------------------------------------
    // 类型查询
    // -----------------------------------------------------------------------

    JsonType type() const { return type_; }
    bool isNull() const { return type_ == JsonType::Null; }
    bool isBool() const { return type_ == JsonType::Bool; }
    bool isInt() const { return type_ == JsonType::Int; }
    bool isDouble() const { return type_ == JsonType::Double; }
    bool isString() const { return type_ == JsonType::String; }
    bool isArray() const { return type_ == JsonType::Array; }
    bool isObject() const { return type_ == JsonType::Object; }
    bool isNumber() const { return type_ == JsonType::Int || type_ == JsonType::Double; }

    // -----------------------------------------------------------------------
    // 值访问（调用前应先检查类型）
    // -----------------------------------------------------------------------

    bool asBool() const { return bool_; }
    std::int64_t asInt() const { return type_ == JsonType::Int ? int_ : static_cast<std::int64_t>(double_); }
    double asDouble() const { return type_ == JsonType::Double ? double_ : static_cast<double>(int_); }
    const std::string& asString() const { return string_; }
    std::string& asString() { return string_; }

    // -----------------------------------------------------------------------
    // Array 操作
    // -----------------------------------------------------------------------

    /// Array 大小
    std::size_t size() const { return array_.size(); }

    /// Array 是否为空
    bool empty() const { return array_.empty() && object_.empty() && string_.empty(); }

    /// Array 访问（无边界检查）
    const JsonValue& at(std::size_t i) const { return array_[i]; }
    JsonValue& at(std::size_t i) { return array_[i]; }

    /// Array 追加
    void append(const JsonValue& v) {
        ensureArray_();
        array_.push_back(v);
    }
    void append(JsonValue&& v) {
        ensureArray_();
        array_.push_back(std::move(v));
    }

    // -----------------------------------------------------------------------
    // Object 操作
    // -----------------------------------------------------------------------

    /// Object 设置键值（覆盖已存在键）
    void set(const std::string& key, const JsonValue& v) {
        ensureObject_();
        for (auto& kv : object_) {
            if (kv.first == key) {
                kv.second = v;
                return;
            }
        }
        object_.emplace_back(key, v);
    }
    void set(const std::string& key, JsonValue&& v) {
        ensureObject_();
        for (auto& kv : object_) {
            if (kv.first == key) {
                kv.second = std::move(v);
                return;
            }
        }
        object_.emplace_back(key, std::move(v));
    }

    /// Object 查找（返回 nullptr 表示不存在）
    const JsonValue* find(const std::string& key) const {
        if (type_ != JsonType::Object) return nullptr;
        for (const auto& kv : object_) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }
    JsonValue* find(const std::string& key) {
        if (type_ != JsonType::Object) return nullptr;
        for (auto& kv : object_) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }

    /// Object 是否包含键
    bool contains(const std::string& key) const { return find(key) != nullptr; }

    /// Object 键值对数量
    std::size_t objectSize() const { return object_.size(); }

    /// Object 迭代器（用于遍历）
    const std::vector<std::pair<std::string, JsonValue>>& objectItems() const { return object_; }
    std::vector<std::pair<std::string, JsonValue>>& objectItems() { return object_; }

    /// Array 元素访问
    const std::vector<JsonValue>& arrayItems() const { return array_; }
    std::vector<JsonValue>& arrayItems() { return array_; }

private:
    void ensureArray_() {
        if (type_ != JsonType::Array) {
            type_ = JsonType::Array;
            array_.clear();
            object_.clear();
            string_.clear();
        }
    }
    void ensureObject_() {
        if (type_ != JsonType::Object) {
            type_ = JsonType::Object;
            object_.clear();
            array_.clear();
            string_.clear();
        }
    }

    JsonType type_ = JsonType::Null;
    bool bool_ = false;
    std::int64_t int_ = 0;
    double double_ = 0.0;
    std::string string_;
    std::vector<JsonValue> array_;
    std::vector<std::pair<std::string, JsonValue>> object_;
};

} // namespace json
} // namespace office
} // namespace zq
