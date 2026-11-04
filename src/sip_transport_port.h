#pragma once
#include <string_view>

namespace axv {
// Changing transports only replaces known defaults. Custom ports are preserved.
inline const wchar_t* suggested_sip_port(bool tls, std::wstring_view current) {
    if (current.empty() || current == L"5060" || current == L"5061")
        return tls ? L"5061" : L"5060";
    return nullptr;
}
} // namespace axv
