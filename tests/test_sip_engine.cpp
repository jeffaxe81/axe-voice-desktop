#include "../src/sip_engine.h"
#include <iostream>
int main() {
    axv::UnavailableSipEngine engine;
    const axv::SipAccount account{L"sip.example.org", L"1001", 5060, false};
    if (engine.status() != axv::SipStatus::Unavailable) return 1;
    if (engine.configure(account) || engine.register_account()) return 2;
    if (engine.start_call(L"sip:1002@example.org")) return 3;
    engine.hangup();
    if (engine.status() != axv::SipStatus::Unavailable || engine.diagnostic().empty()) return 4;
    return 0;
}
