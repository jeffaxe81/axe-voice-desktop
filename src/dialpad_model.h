#pragma once
#include <array>
#include <string>
#include <cstddef>

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
}
