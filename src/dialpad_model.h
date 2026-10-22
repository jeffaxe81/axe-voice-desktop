#pragma once
#include <array>
#include <string>
#include <cstddef>
#include <string_view>
#include <algorithm>

namespace axv {
inline constexpr std::array<wchar_t, 12> dialpad_keys() {
    return {{L'1', L'2', L'3', L'4', L'5', L'6',
             L'7', L'8', L'9', L'*', L'0', L'#'}};
}
// Dialpad buttons never inject SIP metacharacters or control characters.
// Manual keyboard entry is validated by SIP address policy before a call.
inline bool dial_append(std::wstring& destination, wchar_t key, std::size_t limit = 128) {
    bool allowed = false;
    for (wchar_t value : dialpad_keys()) {
        if (value == key) { allowed = true; break; }
    }
    if (!allowed || destination.size() >= limit) return false;
    destination.push_back(key);
    return true;
}
inline void dial_backspace(std::wstring& destination) {
    if (!destination.empty()) destination.pop_back();
}

// Preview keypad edits against the selection; no SIP or media commands.
struct DialEditPlan {
    bool accepted;
    std::size_t start;
    std::size_t end;
    std::wstring replacement;
};
inline DialEditPlan plan_dial_edit(std::wstring_view value,
                                  std::size_t selection_start,
                                  std::size_t selection_end,
                                  wchar_t action, std::size_t limit = 128) {
    const std::size_t bounded_start = std::min(selection_start, value.size());
    const std::size_t bounded_end = std::min(selection_end, value.size());
    std::size_t start = std::min(bounded_start, bounded_end);
    std::size_t end = std::max(bounded_start, bounded_end);
    if (action == L'\0') return {true, 0, value.size(), L""};
    if (action == L'\b') {
        if (start == end && start > 0) --start;
        return {start != end, start, end, L""};
    }
    bool permitted = false;
    for (const auto digit : dialpad_keys()) {
        if (digit == action) { permitted = true; break; }
    }
    if (!permitted || value.size() - (end - start) >= limit)
        return {false, start, end, L""};
    return {true, start, end, std::wstring(1, action)};
}
}
