#pragma once
#include "contact_book.h"
#include <string_view>
namespace axv {
// Returns current model IDs, ordered by favorite, folded name/destination, then ID.
// Matching folds ASCII and Latin-1 letter case without removing accents.
std::vector<std::uint64_t> find_contacts(const ContactBook&,std::wstring_view query,bool favorites_only=false);
}
