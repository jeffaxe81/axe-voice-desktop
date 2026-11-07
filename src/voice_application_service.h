#pragma once
// Shared C++17 application layer for both Windows frontend editions.
//
// This facade never owns native SDK pointers, consumes passwords, sends SIP
// directly, or trusts visibility as authorization. A real BaresipSipEngine
// remains the authoritative owner of native work and revalidates each queued
// command once again on its reactor thread.
//
// Templated on the native port to keep CTest independent of Baresip/.lib and
// allow a future WinUI 3 in-process bridge to reuse the same implementation.
#include "essential_call_actions.h"
#include "native_session_control.h"
#include "destination_feedback.h"
#include <string>
#include <utility>

namespace axv {

struct VoiceApplicationSnapshot {
    SipRuntimeObservation observation{};
    EssentialConnectionSignal signal=EssentialConnectionSignal::Offline;
    EssentialCallActions actions{};
};

template<class NativePort>
class VoiceApplicationService final {
public:
    explicit VoiceApplicationService(NativePort& port) noexcept : port_(port) {}
    VoiceApplicationService(const VoiceApplicationService&)=delete;
    VoiceApplicationService& operator=(const VoiceApplicationService&)=delete;

    VoiceApplicationSnapshot snapshot(bool dial_authorized) const {
        const auto observed=port_.observation();
        return {observed,
                essential_connection_signal(observed,observed.session),
                essential_call_actions(observed,observed.session,dial_authorized)};
    }

    // The destination is still validated and normalized by the native engine.
    // "true" means command submitted to the engine, not SIP connected.
    bool dial(const SipRuntimeObservation& displayed,
              const std::wstring& destination) {
        if(!evaluate_destination(destination).valid())return false;
        const auto current=port_.observation();
        const NativeCallCommand requested{NativeCallAction::Dial,displayed.session,0,{}};
        if(!native_command_is_current(requested,displayed) ||
           !native_command_is_current(requested,current))return false;
        return port_.start_call(destination);
    }

    bool answer(const SipRuntimeObservation& displayed) {
        const auto current=port_.observation();
        if(!native_displayed_call_command_current(
                NativeCallAction::Answer,displayed,current))return false;
        return port_.answer_call(displayed);
    }

    bool hangup(const SipRuntimeObservation& displayed) {
        const auto current=port_.observation();
        if(!native_displayed_call_command_current(
                NativeCallAction::Hangup,displayed,current))return false;
        return port_.hangup_call(displayed);
    }

private:
    NativePort& port_;
};

} // namespace axv
