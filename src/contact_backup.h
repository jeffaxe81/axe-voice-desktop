#pragma once
#include "contact_book.h"
#include <optional>
#include <string_view>
namespace axv {
inline constexpr std::size_t contact_backup_limit=512*1024;
std::optional<std::string> encode_contact_backup(const ContactBook&) noexcept;
std::optional<std::vector<ContactDraft>> decode_contact_backup(std::string_view) noexcept;
}
