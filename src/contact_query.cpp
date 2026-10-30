#include "contact_query.h"

#include <algorithm>
#include <cstddef>

namespace axv {
namespace {
wchar_t fold(wchar_t c) noexcept {
    if ((c >= L'A' && c <= L'Z') ||
        (c >= L'\u00c0' && c <= L'\u00d6') ||
        (c >= L'\u00d8' && c <= L'\u00de')) {
        return static_cast<wchar_t>(c + 0x20);
    }
    return c;
}

bool contains(std::wstring_view text, std::wstring_view query) noexcept {
    if (query.size() > text.size()) return false;
    for (std::size_t start = 0; start <= text.size() - query.size(); ++start) {
        std::size_t offset = 0;
        while (offset < query.size() &&
               fold(text[start + offset]) == fold(query[offset])) ++offset;
        if (offset == query.size()) return true;
    }
    return false;
}

int compare(std::wstring_view a, std::wstring_view b) noexcept {
    const auto length = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < length; ++i) {
        const auto left = fold(a[i]);
        const auto right = fold(b[i]);
        if (left < right) return -1;
        if (left > right) return 1;
    }
    return a.size() < b.size() ? -1 : (a.size() > b.size() ? 1 : 0);
}
} // namespace

std::vector<std::uint64_t> find_contacts(const ContactBook& book,
                                       std::wstring_view query,
                                       bool favorites_only) {
    // Neither permitted field can contain a longer substring. Avoid scanning
    // arbitrarily large caller input; the dialog may supply a 129th character.
    if (query.size() > 128) return {};

    const auto& contacts = book.items();
    std::vector<std::size_t> matches;
    matches.reserve(contacts.size());
    for (std::size_t i = 0; i < contacts.size(); ++i) {
        const auto& contact = contacts[i];
        if (favorites_only && !contact.favorite) continue;
        if (contains(contact.name, query) || contains(contact.destination, query)) {
            matches.push_back(i);
        }
    }
    std::sort(matches.begin(), matches.end(), [&](std::size_t left, std::size_t right) {
        const auto& a = contacts[left];
        const auto& b = contacts[right];
        if (a.favorite != b.favorite) return a.favorite;
        const int name_order = compare(a.name, b.name);
        if (name_order != 0) return name_order < 0;
        const int destination_order = compare(a.destination, b.destination);
        if (destination_order != 0) return destination_order < 0;
        return a.id < b.id;
    });

    std::vector<std::uint64_t> ids;
    ids.reserve(matches.size());
    for (const auto index : matches) ids.push_back(contacts[index].id);
    return ids;
}
} // namespace axv
