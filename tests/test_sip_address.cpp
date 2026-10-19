#include "../src/sip_address.h"
#include <cassert>
int main() {
    using axv::validate_address;
    assert(validate_address(L"1001"));
    assert(validate_address(L"+5511999999999"));
    assert(validate_address(L"sip:1001@example.com"));
    assert(validate_address(L"sips:alice@example.com:5061"));
    assert(!validate_address(L""));
    assert(!validate_address(L"sip:"));
    assert(!validate_address(L"alice@"));
    assert(!validate_address(L"alice@@example.com"));
    assert(!validate_address(L"sip:alice@example.com:0"));
    assert(!validate_address(L"sip:alice@example.com:65536"));
    assert(!validate_address(L"sip:alice@example.com:abc"));
    assert(!validate_address(L"sip:alice@ example.com"));
    assert(!validate_address(L"https://example.com"));
}
