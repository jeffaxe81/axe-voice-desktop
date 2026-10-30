#pragma once
#include <windows.h>
namespace axv {
void show_contact_book(HWND owner,HWND destination_edit) noexcept;
bool contacts_owns_message(const MSG&) noexcept;
bool translate_contacts_message(MSG&) noexcept;
void close_contact_book() noexcept;
}
