#include "../src/sip_address.h"
#include <iostream>
#include <utility>
#include <vector>

int main() {
    const std::vector<std::pair<const wchar_t*, bool>> cases = {
        {L"1001", true}, {L"+5511999999999", true}, {L"*21#", true},
        {L"sip:1001@example.com", true},
        {L"sips:alice@example.com:5061", true},
        {L"", false}, {L"sip:", false}, {L"alice@", false},
        {L"alice@@example.com", false},
        {L"sip:alice@example.com:0", false},
        {L"sip:alice@example.com:65536", false},
        {L"sip:alice@example.com:abc", false},
        {L"sip:alice@ example.com", false},
        {L"https://example.com", false}
    };
    for (const auto& item : cases) {
        if (static_cast<bool>(axv::validate_address(item.first)) != item.second) {
            std::wcerr << L"Unexpected validation result for: " << item.first << L"\n";
            return 1;
        }
    }
    return 0;
}
