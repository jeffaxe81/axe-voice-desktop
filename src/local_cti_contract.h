#pragma once
#include "sip_address.h"
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace axv {

// Local CTI contract. The request body is intentionally a small, strict JSON
// object: {"destination":"1001"} or a SIP/SIPS URI. No extension fields,
// escapes, multiline content or non-ASCII values are accepted in v1.
inline std::optional<std::wstring> parse_cti_call_json(std::string_view body) {
    if (body.empty() || body.size() > 512) return std::nullopt;
    std::size_t i = 0;
    auto skip = [&] {
        while (i < body.size() && (body[i]==' ' || body[i]=='\t' ||
                                  body[i]=='\r' || body[i]=='\n')) ++i;
    };
    const auto consume = [&](char c) {
        skip();
        if (i < body.size() && body[i]==c) { ++i; return true; }
        return false;
    };
    if (!consume('{')) return std::nullopt;
    if (!consume('"')) return std::nullopt;
    constexpr std::string_view key="destination";
    if (body.substr(i,key.size()) != key) return std::nullopt;
    i+=key.size();
    if (!consume('"') || !consume(':') || !consume('"')) return std::nullopt;
    std::wstring destination;
    while (i < body.size() && body[i] != '"') {
        const unsigned char ch=static_cast<unsigned char>(body[i++]);
        if (ch <= 0x20 || ch >= 0x7f || ch == '\\' || destination.size() >= 128)
            return std::nullopt;
        destination.push_back(static_cast<wchar_t>(ch));
    }
    if (!consume('"') || !consume('}')) return std::nullopt;
    skip();
    if (i != body.size()) return std::nullopt;
    const auto result=validate_address(destination);
    if (!result || result.normalized.size() > 128) return std::nullopt;
    return result.normalized;
}

inline bool valid_cti_api_token(std::string_view token) {
    if (token.size() < 32 || token.size() > 128) return false;
    for (unsigned char c: token)
        if (!((c>='A' && c<='Z') || (c>='a' && c<='z') ||
              (c>='0' && c<='9') || c=='-' || c=='_')) return false;
    return true;
}

// Content-type and origin are checked independently from authorization.
// This makes a website unable to issue a browser-originated background POST
// using a form or permissive cross-origin defaults.
inline bool valid_cti_json_type(std::string_view value) {
    return value == "application/json" || value == "application/json; charset=utf-8";
}

} // namespace axv
