#pragma once
#include "sip_runtime_observation.h"

namespace axv {
// Visual status is derived only from native evidence for the current session.
// The process merely running never proves registration.
enum class SipIndicatorTone { Red, Amber, Green };

inline SipIndicatorTone sip_indicator_tone(
    const SipRuntimeObservation& observation, std::uint64_t current_session) noexcept {
    if (observation.process != RuntimeProcess::Running)
        return SipIndicatorTone::Red;
    if (!sip_proof_is_current(observation, current_session))
        return SipIndicatorTone::Amber;
    switch (observation.sip) {
    case SipStatus::Registered:
    case SipStatus::Calling:
    case SipStatus::Ringing:
    case SipStatus::InCall:
        return SipIndicatorTone::Green;
    case SipStatus::Registering:
    case SipStatus::Unavailable:
        return SipIndicatorTone::Amber;
    case SipStatus::Disconnected:
    case SipStatus::Failed:
        return SipIndicatorTone::Red;
    }
    return SipIndicatorTone::Amber;
}
} // namespace axv
