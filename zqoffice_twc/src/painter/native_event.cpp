// =============================================================================
// src/painter/native_event.cpp
//
// NativeEvent* 类族实现
// =============================================================================
#include "painter/native_event.h"

namespace zq {
namespace office {
namespace painter {

// ===========================================================================
// NativeEventBase
// ===========================================================================

NativeEventBase::NativeEventBase() = default;
NativeEventBase::NativeEventBase(const NativeEventBase&) = default;
NativeEventBase::~NativeEventBase() = default;
NativeEventBase& NativeEventBase::operator=(const NativeEventBase&) = default;

// ===========================================================================
// NativeEventComment
// ===========================================================================

NativeEventComment::NativeEventComment() = default;

NativeEventComment::NativeEventComment(const char* author, const char* text)
    : author_(author ? author : ""), text_(text ? text : "") {}

NativeEventComment::NativeEventComment(const NativeEventComment&) = default;
NativeEventComment::NativeEventComment(NativeEventComment&&) noexcept = default;
NativeEventComment::~NativeEventComment() = default;
NativeEventComment& NativeEventComment::operator=(const NativeEventComment&) = default;
NativeEventComment& NativeEventComment::operator=(NativeEventComment&&) noexcept = default;

// ===========================================================================
// NativeEventLink
// ===========================================================================

NativeEventLink::NativeEventLink() = default;

NativeEventLink::NativeEventLink(const char* displayText, const char* url)
    : display_text_(displayText ? displayText : ""), url_(url ? url : "") {}

NativeEventLink::NativeEventLink(const NativeEventLink&) = default;
NativeEventLink::NativeEventLink(NativeEventLink&&) noexcept = default;
NativeEventLink::~NativeEventLink() = default;
NativeEventLink& NativeEventLink::operator=(const NativeEventLink&) = default;
NativeEventLink& NativeEventLink::operator=(NativeEventLink&&) noexcept = default;

// ===========================================================================
// NativeEventSelection
// ===========================================================================

NativeEventSelection::NativeEventSelection() = default;

NativeEventSelection::NativeEventSelection(const char* sheetName, const char* range)
    : sheet_name_(sheetName ? sheetName : ""), range_(range ? range : "") {}

NativeEventSelection::NativeEventSelection(const NativeEventSelection&) = default;
NativeEventSelection::NativeEventSelection(NativeEventSelection&&) noexcept = default;
NativeEventSelection::~NativeEventSelection() = default;
NativeEventSelection& NativeEventSelection::operator=(const NativeEventSelection&) = default;
NativeEventSelection& NativeEventSelection::operator=(NativeEventSelection&&) noexcept = default;

// ===========================================================================
// NativeEventTable
// ===========================================================================

NativeEventTable::NativeEventTable() = default;

NativeEventTable::NativeEventTable(const char* tableId, const char* range)
    : table_id_(tableId ? tableId : ""), range_(range ? range : "") {}

NativeEventTable::NativeEventTable(const NativeEventTable&) = default;
NativeEventTable::NativeEventTable(NativeEventTable&&) noexcept = default;
NativeEventTable::~NativeEventTable() = default;
NativeEventTable& NativeEventTable::operator=(const NativeEventTable&) = default;
NativeEventTable& NativeEventTable::operator=(NativeEventTable&&) noexcept = default;

// ===========================================================================
// NativeEventListener
// ===========================================================================

NativeEventListener::NativeEventListener() = default;
NativeEventListener::NativeEventListener(const NativeEventListener&) = default;
NativeEventListener::~NativeEventListener() = default;
NativeEventListener& NativeEventListener::operator=(const NativeEventListener&) = default;

void NativeEventListener::OnComment(const NativeEventComment&) {}
void NativeEventListener::OnLink(const NativeEventLink&) {}
void NativeEventListener::OnSelection(const NativeEventSelection&) {}
void NativeEventListener::OnTable(const NativeEventTable&) {}

}  // namespace painter
}  // namespace office
}  // namespace zq
