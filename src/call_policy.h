#pragma once
#include "sip_engine.h"
namespace axv {
enum class CallAction { Dial, Answer, Hangup };
inline bool can_apply_call_action(SipStatus status, CallAction action) {
    switch(action) {
    case CallAction::Dial: return status == SipStatus::Registered;
    case CallAction::Answer: return status == SipStatus::Ringing;
    case CallAction::Hangup: return status == SipStatus::Calling || status == SipStatus::Ringing ||
                                    status == SipStatus::InCall;
    }
    return false;
}
}
