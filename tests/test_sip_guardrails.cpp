#include "../src/sip_registration_policy.h"
#include "../src/sip_security_policy.h"
#include <iostream>
int main() {
    using namespace axv;
    const SipAccount valid{L"pbx.example.com",L"1001",5061,true};
    const SipAccount invalid{L"",L"1001",5060,false};
    if (evaluate_registration(invalid,SipStatus::Disconnected) != RegistrationDecision::InvalidAccount) return 1;
    if (evaluate_registration(valid,SipStatus::Unavailable) != RegistrationDecision::EngineUnavailable) return 2;
    if (evaluate_registration(valid,SipStatus::Registering) != RegistrationDecision::Busy) return 3;
    if (evaluate_registration(valid,SipStatus::Registered) != RegistrationDecision::AlreadyRegistered) return 4;
    if (evaluate_registration(valid,SipStatus::Disconnected) != RegistrationDecision::Allowed) return 5;
    if (account_security_level(valid) != SecurityLevel::SignalingTls) return 6;
    if (is_media_encryption_confirmed(valid)) return 7;
    return 0;
}
