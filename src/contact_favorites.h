#pragma once
#include "contact_book.h"
namespace axv {
ContactResult toggle_contact_favorite(ContactBook&,std::uint64_t) noexcept;
std::size_t favorite_contact_count(const ContactBook&) noexcept;
}
