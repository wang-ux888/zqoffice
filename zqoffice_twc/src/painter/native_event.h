// =============================================================================
// src/painter/native_event.h
//
// zq::office::painter::NativeEvent* 事件回调类族
//   基类 NativeEventBase（抽象）+ 4 个具体事件 + 1 个监听器
//   所有事件均带 vtable，可跨 reader 复用
//
// =============================================================================
#pragma once

#include <memory>
#include <string>

namespace zq {
namespace office {
namespace painter {

// ---------------------------------------------------------------------------
// NativeEventBase（抽象基类）
// ---------------------------------------------------------------------------
class NativeEventBase {
public:
    NativeEventBase();
    NativeEventBase(const NativeEventBase& other);
    virtual ~NativeEventBase();

    NativeEventBase& operator=(const NativeEventBase& other);
};

// ---------------------------------------------------------------------------
// NativeEventComment（评论事件）
// ---------------------------------------------------------------------------
class NativeEventComment : public NativeEventBase {
public:
    NativeEventComment();
    NativeEventComment(const char* author, const char* text);
    NativeEventComment(const NativeEventComment& other);
    NativeEventComment(NativeEventComment&& other) noexcept;
    ~NativeEventComment() override;

    NativeEventComment& operator=(const NativeEventComment& other);
    NativeEventComment& operator=(NativeEventComment&& other) noexcept;

    const std::string& GetAuthor() const { return author_; }
    const std::string& GetText() const { return text_; }

    void SetAuthor(const std::string& author) { author_ = author; }
    void SetText(const std::string& text) { text_ = text; }

private:
    std::string author_;
    std::string text_;
};

// ---------------------------------------------------------------------------
// NativeEventLink（超链接事件）
// ---------------------------------------------------------------------------
class NativeEventLink : public NativeEventBase {
public:
    NativeEventLink();
    NativeEventLink(const char* displayText, const char* url);
    NativeEventLink(const NativeEventLink& other);
    NativeEventLink(NativeEventLink&& other) noexcept;
    ~NativeEventLink() override;

    NativeEventLink& operator=(const NativeEventLink& other);
    NativeEventLink& operator=(NativeEventLink&& other) noexcept;

    const std::string& GetDisplayText() const { return display_text_; }
    const std::string& GetUrl() const { return url_; }

    void SetDisplayText(const std::string& text) { display_text_ = text; }
    void SetUrl(const std::string& url) { url_ = url; }

private:
    std::string display_text_;
    std::string url_;
};

// ---------------------------------------------------------------------------
// NativeEventSelection（选区事件）
// ---------------------------------------------------------------------------
class NativeEventSelection : public NativeEventBase {
public:
    NativeEventSelection();
    NativeEventSelection(const char* sheetName, const char* range);
    NativeEventSelection(const NativeEventSelection& other);
    NativeEventSelection(NativeEventSelection&& other) noexcept;
    ~NativeEventSelection() override;

    NativeEventSelection& operator=(const NativeEventSelection& other);
    NativeEventSelection& operator=(NativeEventSelection&& other) noexcept;

    const std::string& GetSheetName() const { return sheet_name_; }
    const std::string& GetRange() const { return range_; }

    void SetSheetName(const std::string& name) { sheet_name_ = name; }
    void SetRange(const std::string& range) { range_ = range; }

private:
    std::string sheet_name_;
    std::string range_;
};

// ---------------------------------------------------------------------------
// NativeEventTable（表格事件）
// ---------------------------------------------------------------------------
class NativeEventTable : public NativeEventBase {
public:
    NativeEventTable();
    NativeEventTable(const char* tableId, const char* range);
    NativeEventTable(const NativeEventTable& other);
    NativeEventTable(NativeEventTable&& other) noexcept;
    ~NativeEventTable() override;

    NativeEventTable& operator=(const NativeEventTable& other);
    NativeEventTable& operator=(NativeEventTable&& other) noexcept;

    const std::string& GetTableId() const { return table_id_; }
    const std::string& GetRange() const { return range_; }

    void SetTableId(const std::string& id) { table_id_ = id; }
    void SetRange(const std::string& range) { range_ = range; }

private:
    std::string table_id_;
    std::string range_;
};

// ---------------------------------------------------------------------------
// NativeEventListener（监听器）
// ---------------------------------------------------------------------------
class NativeEventListener {
public:
    NativeEventListener();
    NativeEventListener(const NativeEventListener& other);
    virtual ~NativeEventListener();

    NativeEventListener& operator=(const NativeEventListener& other);

    // 事件回调（子类重写）
    virtual void OnComment(const NativeEventComment& event);
    virtual void OnLink(const NativeEventLink& event);
    virtual void OnSelection(const NativeEventSelection& event);
    virtual void OnTable(const NativeEventTable& event);
};

}  // namespace painter
}  // namespace office
}  // namespace zq
