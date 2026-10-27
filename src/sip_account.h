#pragma once
#include "sip_engine.h"
#include "sip_host.h"
#include <cwctype>
#include <string>

namespace axv {
enum class AccountError { None, MissingServer, InvalidServer, MissingUser, InvalidUser, InvalidPort };
inline AccountError validate_account(const SipAccount& a) {
    if (a.server.empty()) return AccountError::MissingServer;
    if (a.username.empty()) return AccountError::MissingUser;
    if (a.port == 0) return AccountError::InvalidPort;
    if (!valid_sip_host(a.server)) return AccountError::InvalidServer;
    for (wchar_t c : a.username) {
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'_' || c == L'.' ||
              c == L'-' || c == L'+'))
            return AccountError::InvalidUser;
    }
    return AccountError::None;
}
}
