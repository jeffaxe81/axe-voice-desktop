#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "contact_ui_actions.h"
#include <cstring>

namespace axv {
bool copy_contact_destination(HWND owner, const Contact& contact) noexcept {
    if (!owner) return false;
    try {
        const ContactDraft draft{contact.name, contact.destination, contact.favorite};
        if (!valid_contact_draft(draft)) return false;

        // Validation bounds this allocation to 129 UTF-16 code units.
        const auto bytes = (draft.destination.size() + 1) * sizeof(wchar_t);
        const auto memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (!memory) return false;
        auto* data = GlobalLock(memory);
        if (!data) {
            GlobalFree(memory);
            return false;
        }
        std::memcpy(data, draft.destination.c_str(), bytes);
        GlobalUnlock(memory);
        if (!OpenClipboard(owner)) {
            GlobalFree(memory);
            return false;
        }
        const bool copied = EmptyClipboard() &&
                            SetClipboardData(CF_UNICODETEXT, memory) != nullptr;
        CloseClipboard();
        // The system owns the allocation only after SetClipboardData succeeds.
        if (!copied) GlobalFree(memory);
        return copied;
    } catch (...) {
        return false;
    }
}
}
