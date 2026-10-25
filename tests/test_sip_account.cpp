#include "../src/sip_account.h"
#include <iostream>
int main() {
    using namespace axv;
    if (validate_account({L"pbx.example.com", L"1001", 5060, false}) != AccountError::None) return 1;
    if (validate_account({L"pbx.example.com", L"1001", 5061, true}) != AccountError::None) return 2;
    if (validate_account({L"", L"1001", 5060, false}) != AccountError::MissingServer) return 3;
    if (validate_account({L"pbx.example.com", L"", 5060, false}) != AccountError::MissingUser) return 4;
    if (validate_account({L"pbx example.com", L"1001", 5060, false}) != AccountError::InvalidServer) return 5;
    if (validate_account({L"pbx.example.com", L"user@domain", 5060, false}) != AccountError::InvalidUser) return 6;
    if (validate_account({L"pbx.example.com", L"1001", 0, false}) != AccountError::InvalidPort) return 7;
    return 0;
}
