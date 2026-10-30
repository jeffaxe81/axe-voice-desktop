#include "contact_ui_actions.h"

namespace axv {

bool use_contact_destination(HWND destination_edit, const Contact& contact) noexcept {
    try {
        // Revalidate even persisted contacts before writing to the call UI.
        if (!valid_contact_draft({contact.name, contact.destination, contact.favorite}) ||
            !IsWindow(destination_edit))
            return false;

        wchar_t class_name[32]{};
        if (!GetClassNameW(destination_edit, class_name, 32) ||
            lstrcmpiW(class_name, L"EDIT") != 0)
            return false;

        const auto limit = SendMessageW(destination_edit, EM_GETLIMITTEXT, 0, 0);
        if (limit < 0 || contact.destination.size() > static_cast<std::size_t>(limit))
            return false;

        // SetWindowText sends the edit's normal change notification. Never send
        // a call command: the user must review the unchanged destination first.
        if (!SetWindowTextW(destination_edit, contact.destination.c_str()))
            return false;

        const HWND owner = GetAncestor(destination_edit, GA_ROOT);
        if (owner) {
            SendMessageW(owner,contact_dialer_review_message,0,0);
            if (IsIconic(owner)) ShowWindow(owner, SW_RESTORE);
            SetForegroundWindow(owner);
        }
        SetFocus(destination_edit);
        SendMessageW(destination_edit, EM_SETSEL, 0, -1);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace axv
