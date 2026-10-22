#pragma once
#ifndef _WIN32
#error Windows-only clipboard helper
#endif
#include <windows.h>
#include <cstring>
#include <limits>
#include <string>
namespace axv {
// GUI-thread helper: CF_UNICODETEXT takes ownership only on successful SetClipboardData.
inline bool copy_unicode_clipboard(HWND window, const std::wstring& value) {
    if (value.empty() || value.size() > 8192 ||
        value.size() >= (std::numeric_limits<std::size_t>::max)()/sizeof(wchar_t))
        return false;
    const auto bytes = (value.size()+1)*sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) return false;
    void* buffer = GlobalLock(memory);
    if (!buffer) { GlobalFree(memory); return false; }
    std::memcpy(buffer, value.c_str(), bytes);
    GlobalUnlock(memory);
    if (!OpenClipboard(window)) { GlobalFree(memory); return false; }
    if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, memory)) {
        CloseClipboard();
        GlobalFree(memory);
        return false;
    }
    CloseClipboard();
    return true;
}
} // namespace axv
