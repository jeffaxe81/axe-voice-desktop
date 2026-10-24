#pragma once
// Optional Windows desktop conveniences. No SIP registration/call authorization.
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include "destination_feedback.h"

namespace axv {
inline constexpr wchar_t kDesktopRunKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline constexpr wchar_t kDesktopRunValue[] = L"AXEVoiceDesktop";

inline std::wstring desktop_executable_command() {
    wchar_t exe[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, exe, static_cast<DWORD>(std::size(exe)));
    if (length == 0 || length >= std::size(exe)) return {};
    return L"\"" + std::wstring(exe, length) + L"\"";
}
inline bool desktop_autostart_enabled() {
    const auto expected = desktop_executable_command();
    if (expected.empty()) return false;
    wchar_t value[32768] = {};
    DWORD bytes = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kDesktopRunKey, kDesktopRunValue,
                     RRF_RT_REG_SZ, nullptr, value, &bytes) != ERROR_SUCCESS) return false;
    return expected == value;
}
inline bool desktop_set_autostart(bool enabled) {
    const auto command = desktop_executable_command();
    if (command.empty()) return false;
    HKEY key = nullptr;
    if (enabled) {
        if (RegCreateKeyExW(HKEY_CURRENT_USER, kDesktopRunKey, 0, nullptr, 0,
                            KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
            return false;
        const auto bytes = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
        const LONG rc = RegSetValueExW(key, kDesktopRunValue, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()), bytes);
        RegCloseKey(key);
        return rc == ERROR_SUCCESS;
    }
    if (!desktop_autostart_enabled()) return true;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kDesktopRunKey, 0,
                      KEY_SET_VALUE, &key) != ERROR_SUCCESS) return false;
    const LONG rc = RegDeleteValueW(key, kDesktopRunValue);
    RegCloseKey(key);
    return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
}
inline std::wstring valid_dial_destination(const std::wstring& value) {
    if (value.size() > 128 || !evaluate_destination(value).valid()) return {};
    return value;
}
inline std::wstring load_desktop_favorite(const wchar_t* preferences_key) {
    wchar_t value[130] = {};
    DWORD bytes = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, preferences_key, L"FavoriteDestination",
                     RRF_RT_REG_SZ, nullptr, value, &bytes) != ERROR_SUCCESS)
        return {};
    return valid_dial_destination(value);
}
inline bool save_desktop_favorite(const wchar_t* preferences_key,
                                  const std::wstring& value) {
    if (valid_dial_destination(value).empty()) return false;
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, preferences_key, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) return false;
    const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LONG rc = RegSetValueExW(key, L"FavoriteDestination", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()), bytes);
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}
// Session-only destination editing history. It is NOT a call log.
inline void remember_dial_destination(std::vector<std::wstring>& recent,
                                       const std::wstring& value) {
    const auto accepted = valid_dial_destination(value);
    if (accepted.empty()) return;
    recent.erase(std::remove(recent.begin(), recent.end(), accepted), recent.end());
    recent.insert(recent.begin(), accepted);
    if (recent.size() > 10) recent.resize(10);
}
} // namespace axv
