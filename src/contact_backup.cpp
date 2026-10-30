#include "contact_backup.h"
#include <climits>
#include <cstdint>
#include <utility>

namespace axv {
namespace {
void utf8(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) out.push_back(static_cast<char>(cp));
    else if (cp < 0x800) { out.push_back(static_cast<char>(0xc0 | (cp >> 6))); out.push_back(static_cast<char>(0x80 | (cp & 63))); }
    else if (cp < 0x10000) { out.push_back(static_cast<char>(0xe0 | (cp >> 12))); out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63))); out.push_back(static_cast<char>(0x80 | (cp & 63))); }
    else { out.push_back(static_cast<char>(0xf0 | (cp >> 18))); out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 63))); out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 63))); out.push_back(static_cast<char>(0x80 | (cp & 63))); }
}
void quoted(std::string& out, const std::wstring& value) {
    out.push_back('"');
    for (std::size_t i = 0; i < value.size(); ++i) {
        auto cp = static_cast<std::uint32_t>(value[i]);
#if WCHAR_MAX <= 0xffff
        if (cp >= 0xd800 && cp <= 0xdbff) cp = 0x10000 + ((cp - 0xd800) << 10) + static_cast<std::uint32_t>(value[++i]) - 0xdc00;
#endif
        if (cp == '"' || cp == '\\') out.push_back('\\');
        utf8(out, cp);
    }
    out.push_back('"');
}
class Parser {
    std::string_view input;
    std::size_t pos = 0;
    void space() { while (pos < input.size() && (input[pos] == ' ' || input[pos] == '\r' || input[pos] == '\n' || input[pos] == '\t')) ++pos; }
    bool take(char c) { space(); if (pos == input.size() || input[pos] != c) return false; ++pos; return true; }
    bool literal(std::string_view s) { space(); if (input.substr(pos, s.size()) != s) return false; pos += s.size(); return true; }
    bool hex(std::uint32_t& cp) {
        cp = 0;
        for (int n = 0; n < 4; ++n) {
            if (pos == input.size()) return false;
            unsigned char c = input[pos++];
            unsigned v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
            if (v == 16) return false;
            cp = (cp << 4) | v;
        }
        return true;
    }
    bool codepoint(std::uint32_t& cp) {
        if (pos == input.size()) return false;
        auto first = static_cast<unsigned char>(input[pos++]);
        if (first < 0x80) { cp = first; return true; }
        unsigned count; std::uint32_t minimum;
        if (first >= 0xc2 && first <= 0xdf) { count = 1; cp = first & 31; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { count = 2; cp = first & 15; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { count = 3; cp = first & 7; minimum = 0x10000; }
        else return false;
        while (count--) {
            if (pos == input.size()) return false;
            auto c = static_cast<unsigned char>(input[pos++]);
            if ((c & 0xc0) != 0x80) return false;
            cp = (cp << 6) | (c & 63);
        }
        return cp >= minimum && cp <= 0x10ffff && !(cp >= 0xd800 && cp <= 0xdfff);
    }
    bool string(std::wstring& out, std::size_t limit, bool ascii = false) {
        if (!take('"')) return false;
        std::size_t units = 0;
        while (pos < input.size()) {
            if (input[pos] == '"') { ++pos; return true; }
            std::uint32_t cp;
            if (input[pos] == '\\') {
                ++pos;
                if (pos == input.size()) return false;
                char escape = input[pos++];
                if (escape == '"' || escape == '\\' || escape == '/') cp = static_cast<unsigned char>(escape);
                else if (escape == 'u') {
                    if (!hex(cp)) return false;
                    if (cp >= 0xd800 && cp <= 0xdbff) {
                        if (input.substr(pos, 2) != "\\u") return false;
                        pos += 2; std::uint32_t low;
                        if (!hex(low) || low < 0xdc00 || low > 0xdfff) return false;
                        cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
                    } else if (cp >= 0xdc00 && cp <= 0xdfff) return false;
                } else return false; // All remaining JSON escapes denote forbidden controls.
            } else if (!codepoint(cp)) return false;
            if (cp < 0x20 || (cp >= 0x7f && cp <= 0x9f) || (ascii && cp >= 0x7f)) return false;
            units += cp > 0xffff ? 2 : 1;
            if (units > limit) return false;
#if WCHAR_MAX <= 0xffff
            if (cp > 0xffff) { out.push_back(static_cast<wchar_t>(0xd800 + ((cp - 0x10000) >> 10))); out.push_back(static_cast<wchar_t>(0xdc00 + ((cp - 0x10000) & 1023))); }
            else out.push_back(static_cast<wchar_t>(cp));
#else
            out.push_back(static_cast<wchar_t>(cp));
#endif
        }
        return false;
    }
    bool contact(ContactDraft& draft) {
        if (!take('{')) return false;
        unsigned fields = 0;
        do {
            std::wstring key;
            if (!string(key, 11, true) || !take(':')) return false;
            unsigned bit = key == L"name" ? 1 : key == L"destination" ? 2 : key == L"favorite" ? 4 : 0;
            if (!bit || (fields & bit)) return false;
            fields |= bit;
            if (bit == 1) { if (!string(draft.name, 64)) return false; }
            else if (bit == 2) { if (!string(draft.destination, 128, true)) return false; }
            else if (literal("true")) draft.favorite = true;
            else if (literal("false")) draft.favorite = false;
            else return false;
        } while (take(','));
        return take('}') && fields == 7 && valid_contact_draft(draft);
    }
    bool contacts(std::vector<ContactDraft>& out) {
        if (!take('[')) return false;
        if (take(']')) return true;
        do {
            if (out.size() == ContactBook::capacity) return false;
            ContactDraft draft;
            if (!contact(draft)) return false;
            for (const auto& existing : out) if (existing.destination == draft.destination) return false;
            out.push_back(std::move(draft));
        } while (take(','));
        return take(']');
    }
public:
    explicit Parser(std::string_view bytes) : input(bytes) {}
    bool parse(std::vector<ContactDraft>& out) {
        if (!take('{')) return false;
        unsigned fields = 0;
        do {
            std::wstring key;
            if (!string(key, 8, true) || !take(':')) return false;
            unsigned bit = key == L"version" ? 1 : key == L"contacts" ? 2 : 0;
            if (!bit || (fields & bit)) return false;
            fields |= bit;
            if (bit == 1) { if (!literal("1")) return false; }
            else if (!contacts(out)) return false;
        } while (take(','));
        if (!take('}') || fields != 3) return false;
        space(); return pos == input.size();
    }
};
}
std::optional<std::string> encode_contact_backup(const ContactBook& book) noexcept {
    try {
        if (book.items().size() > ContactBook::capacity) return std::nullopt;
        std::string out = "{\"version\":1,\"contacts\":[";
        bool first = true;
        for (const auto& contact : book.items()) {
            ContactDraft draft{contact.name, contact.destination, contact.favorite};
            if (!valid_contact_draft(draft)) return std::nullopt;
            if (!first) out.push_back(',');
            first = false;
            out += "{\"name\":"; quoted(out, contact.name);
            out += ",\"destination\":"; quoted(out, contact.destination);
            out += contact.favorite ? ",\"favorite\":true}" : ",\"favorite\":false}";
            if (out.size() > contact_backup_limit) return std::nullopt;
        }
        out += "]}";
        if (out.size() > contact_backup_limit) return std::nullopt;
        return out;
    } catch (...) { return std::nullopt; }
}
std::optional<std::vector<ContactDraft>> decode_contact_backup(std::string_view bytes) noexcept {
    try {
        if (bytes.size() > contact_backup_limit) return std::nullopt;
        std::vector<ContactDraft> contacts;
        if (!Parser(bytes).parse(contacts)) return std::nullopt;
        return contacts;
    } catch (...) { return std::nullopt; }
}
}
