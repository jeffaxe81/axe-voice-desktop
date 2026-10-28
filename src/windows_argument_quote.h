#pragma once
#include <string>
#include <string_view>
namespace axv {
// One always-quoted Windows CRT argument; no shell interpretation is involved.
inline std::wstring quote_windows_argument(std::wstring_view value) {
    std::wstring quoted=L"\"";
    std::size_t slashes=0;
    for(const auto c:value) {
        if(c==L'\\'){++slashes;continue;}
        quoted.append(c==L'"'?slashes*2+1:slashes,L'\\');
        quoted+=c;slashes=0;
    }
    quoted.append(slashes*2,L'\\');
    quoted+=L'"';
    return quoted;
}
}
