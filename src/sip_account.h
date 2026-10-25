#pragma once
#include "sip_engine.h"
#include <cwctype>
#include <string>

namespace axv {
enum class AccountError { None, MissingServer, InvalidServer, MissingUser, InvalidUser, InvalidPort };
inline AccountError validate_account(const SipAccount& a) {
    if (a.server.empty()) return AccountError::MissingServer;
    if (a.username.empty()) return AccountError::MissingUser;
    if (a.port == 0) return AccountError::InvalidPort;
    if (a.server.front() == L'.' || a.server.back() == L'.' ||
        a.server.front() == L'-' || a.server.back() == L'-')
        return AccountError::InvalidServer;
    for (wchar_t c : a.server) {
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'.' || c == L'-'))
            return AccountError::InvalidServer;
    }
    for (wchar_t c : a.username) {
        if (std::iswspace(c) || std::iswcntrl(c) || c == L'@' ||
            c == L'<' || c == L'>' || c == L'"' || c == L':')
            return AccountError::InvalidUser;
    }
    return AccountError::None;
}
}
