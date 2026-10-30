#include "contact_book.h"
#include "sip_address.h"
#include <algorithm>
#include <climits>
#include <limits>
#include <utility>

namespace axv {
namespace {
bool valid_name(const std::wstring& name) noexcept {
    if (name.empty()) return false;
    std::size_t units = 0;
    bool visible = false;
    for (std::size_t i = 0; i < name.size(); ++i) {
        auto cp = static_cast<std::uint32_t>(name[i]);
#if WCHAR_MAX <= 0xffff
        if (cp >= 0xd800 && cp <= 0xdbff) {
            if (++i == name.size()) return false;
            const auto low = static_cast<std::uint32_t>(name[i]);
            if (low < 0xdc00 || low > 0xdfff) return false;
            cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
        } else if (cp >= 0xdc00 && cp <= 0xdfff) return false;
#else
        if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
#endif
        units += cp > 0xffff ? 2 : 1;
        if (units > 64 || cp < 0x20 || (cp >= 0x7f && cp <= 0x9f) ||
            cp == 0x061c || (cp >= 0x200b && cp <= 0x200f) ||
            (cp >= 0x2028 && cp <= 0x202e) || (cp >= 0x2060 && cp <= 0x206f) || cp == 0xfeff)
            return false;
        if (cp != 0x20 && cp != 0xa0 && cp != 0x1680 &&
            !(cp >= 0x2000 && cp <= 0x200a) && cp != 0x202f && cp != 0x205f && cp != 0x3000)
            visible = true;
    }
    return visible;
}
bool valid_draft(const ContactDraft& draft) {
    if (!valid_name(draft.name) || draft.destination.empty() || draft.destination.size() > 128) return false;
    const auto& destination = draft.destination;
    for (wchar_t c : destination) if (c <= 32 || c >= 127) return false;
    if (!validate_address(destination)) return false;
    const auto at = destination.find(L'@');
    if (at != std::wstring::npos) {
        const std::size_t start = destination.compare(0, 5, L"sips:") == 0 ? 5 :
                                  destination.compare(0, 4, L"sip:") == 0 ? 4 : 0;
        for (std::size_t i = start; i < at; ++i) {
            const auto c = destination[i];
            if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
                  (c >= L'0' && c <= L'9') || c == L'_' || c == L'.' ||
                  c == L'-' || c == L'+' || c == L'*' || c == L'#')) return false;
        }
    }
    return true;
}
}
bool valid_contact_draft(const ContactDraft& draft) noexcept {
    try { return valid_draft(draft); } catch (...) { return false; }
}
const Contact* ContactBook::find(std::uint64_t id) const noexcept {
    for (const auto& contact : items_) if (contact.id == id) return &contact;
    return nullptr;
}
ContactResult ContactBook::add(const ContactDraft& draft) noexcept {
    try {
        if (!valid_draft(draft)) return ContactResult::Invalid;
        for (const auto& contact : items_) if (contact.destination == draft.destination) return ContactResult::Duplicate;
        if (items_.size() >= capacity) return ContactResult::Full;
        if (next_id_ == 0) return ContactResult::Error;
        Contact prepared{next_id_, draft.name, draft.destination, draft.favorite};
        items_.push_back(std::move(prepared));
        ++next_id_; // Zero marks exhaustion; previously issued IDs are never recycled.
        return ContactResult::Ok;
    } catch (...) { return ContactResult::Error; }
}
ContactResult ContactBook::update(std::uint64_t id, const ContactDraft& draft) noexcept {
    try {
        auto target = std::find_if(items_.begin(), items_.end(), [id](const Contact& c) { return c.id == id; });
        if (target == items_.end()) return ContactResult::NotFound;
        if (!valid_draft(draft)) return ContactResult::Invalid;
        for (const auto& contact : items_)
            if (contact.id != id && contact.destination == draft.destination) return ContactResult::Duplicate;
        Contact prepared{id, draft.name, draft.destination, draft.favorite};
        target->name.swap(prepared.name);
        target->destination.swap(prepared.destination);
        target->favorite = prepared.favorite;
        return ContactResult::Ok;
    } catch (...) { return ContactResult::Error; }
}
ContactResult ContactBook::remove(std::uint64_t id) noexcept {
    auto target = std::find_if(items_.begin(), items_.end(), [id](const Contact& c) { return c.id == id; });
    if (target == items_.end()) return ContactResult::NotFound;
    items_.erase(target);
    return ContactResult::Ok;
}
ContactResult ContactBook::set_favorite(std::uint64_t id, bool favorite) noexcept {
    for (auto& contact : items_) if (contact.id == id) {
        contact.favorite = favorite;
        return ContactResult::Ok;
    }
    return ContactResult::NotFound;
}
void ContactBook::swap(ContactBook& other) noexcept {
    items_.swap(other.items_);
    std::swap(next_id_, other.next_id_);
}
}
