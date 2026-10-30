#pragma once
#include <windows.h>
#include "contact_store.h"
namespace axv {
inline constexpr UINT contact_dialer_review_message=WM_APP+0x279;
enum class ContactTransferStatus {Cancelled,Ok,Invalid,Conflict,Error};
struct ContactTransferResult {ContactTransferStatus status=ContactTransferStatus::Cancelled;std::size_t added=0,skipped=0;};
ContactTransferResult import_contacts(HWND,const ContactBook&,ContactBook&,ContactStore&) noexcept;
ContactTransferResult export_contacts(HWND,const ContactBook&) noexcept;
bool copy_contact_destination(HWND,const Contact&) noexcept;
bool use_contact_destination(HWND,const Contact&) noexcept;
}
