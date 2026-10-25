#pragma once
#include "sip_engine.h"
#include "sip_account.h"
namespace axv {
enum class RegistrationDecision { Allowed, InvalidAccount, EngineUnavailable, Busy, AlreadyRegistered };
inline RegistrationDecision evaluate_registration(const SipAccount& account, SipStatus state) {
    if (validate_account(account) != AccountError::None) return RegistrationDecision::InvalidAccount;
    if (state == SipStatus::Unavailable) return RegistrationDecision::EngineUnavailable;
    if (state == SipStatus::Registering || state == SipStatus::Calling || state == SipStatus::Ringing || state == SipStatus::InCall)
        return RegistrationDecision::Busy;
    if (state == SipStatus::Registered) return RegistrationDecision::AlreadyRegistered;
    return RegistrationDecision::Allowed;
}
}
