#include "sip_address.h"
#include <cwctype>

namespace axv {
AddressResult validate_address(const std::wstring& input) {
    if (input.empty()) return {AddressError::Empty, L""};
    for (wchar_t c : input) {
        if (std::iswspace(c) || std::iswcntrl(c)) return {AddressError::Whitespace, L""};
    }
    std::wstring value = input;
    auto colon = value.find(L':');
    if (colon != std::wstring::npos) {
        auto scheme = value.substr(0, colon);
        if (scheme != L"sip" && scheme != L"sips") return {AddressError::InvalidScheme, L""};
        value = value.substr(colon + 1);
    }
    if (value.empty()) return {AddressError::InvalidUser, L""};
    if (value.find_first_of(L"<>\\\"?;") != std::wstring::npos) return {AddressError::InvalidUser, L""};
    auto at = value.find(L'@');
    if (at == std::wstring::npos) {
        // Bare dial strings are allowed. The SIP backend will apply account routing.
        for (wchar_t c : value) {
            if (!std::iswdigit(c) && c != L'+' && c != L'*' && c != L'#')
                return {AddressError::InvalidUser, L""};
        }
        return {AddressError::None, colon == std::wstring::npos ? value : input};
    }
    if (at == 0 || at == value.size() - 1 || value.find(L'@', at + 1) != std::wstring::npos)
        return {AddressError::InvalidHost, L""};
    std::wstring host = value.substr(at + 1);
    if (host.front() == L'.' || host.back() == L'.') return {AddressError::InvalidHost, L""};
    for (wchar_t c : host) {
        if (!std::iswalnum(c) && c != L'-' && c != L'.' && c != L':')
            return {AddressError::InvalidHost, L""};
    }
    auto portColon = host.find(L':');
    if (portColon != std::wstring::npos) {
        std::wstring port = host.substr(portColon + 1);
        if (port.empty() || port.size() > 5) return {AddressError::InvalidPort, L""};
        unsigned long number = 0;
        for (wchar_t c : port) {
            if (c < L'0' || c > L'9') return {AddressError::InvalidPort, L""};
            number = number * 10 + static_cast<unsigned long>(c - L'0');
        }
        if (number == 0 || number > 65535) return {AddressError::InvalidPort, L""};
    }
    return {AddressError::None, input.find(L':') == std::wstring::npos ? L"sip:" + value : input};
}
}
