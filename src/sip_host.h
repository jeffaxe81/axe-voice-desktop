#pragma once
#include <string>
namespace axv {
// Accept IPv4 or an ASCII DNS/FQDN host, without schemes, ports or credentials.
// Syntax validation only: DNS resolution and SIP reachability are checked on registration.
inline bool valid_sip_host(const std::wstring& host) {
    if (host.empty() || host.size() > 253) return false;
    std::size_t label = 0;
    bool last_alnum = false;
    bool all_numeric_dots = true;
    unsigned octet = 0;
    unsigned octets = 1;
    bool valid_ipv4 = true;
    for (wchar_t c : host) {
        const bool digit = c >= L'0' && c <= L'9';
        const bool alpha = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z');
        const bool alnum = alpha || digit;
        if (c == L'.') {
            if (!label || !last_alnum) return false;
            if (label > 3 || octet > 255) valid_ipv4 = false;
            ++octets;
            label = 0;
            last_alnum = false;
            octet = 0;
        } else {
            if (!alnum && (c != L'-' || !label)) return false;
            if (++label > 63) return false;
            last_alnum = alnum;
            if (!digit) all_numeric_dots = false;
            if (digit && octet <= 255)
                octet = octet * 10 + static_cast<unsigned>(c - L'0');
            else if (digit) valid_ipv4 = false;
        }
    }
    if (!label || !last_alnum) return false;
    if (!all_numeric_dots) return true;  // DNS host, optionally a single label.
    return valid_ipv4 && octets == 4 && label <= 3 && octet <= 255;
}
} // namespace axv
