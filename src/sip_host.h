#pragma once
#include <string>
namespace axv {
// DNS/IPv4-shaped host syntax only; never performs name resolution.
inline bool valid_sip_host(const std::wstring& host) {
    if(host.empty() || host.size()>253) return false;
    std::size_t label=0;
    bool last_alnum=false;
    for(wchar_t c:host) {
        const bool alnum=(c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9');
        if(c==L'.') {
            if(!label || !last_alnum) return false;
            label=0;last_alnum=false;
        } else {
            if(!alnum && (c!=L'-' || !label)) return false;
            if(++label>63) return false;
            last_alnum=alnum;
        }
    }
    return label && last_alnum;
}
}
