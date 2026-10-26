#pragma once
#include "sip_runtime_observation.h"
#include <cstdint>

namespace axv {
struct CallDurationSnapshot {
    bool active = false;
    std::uint64_t elapsed_ms = 0;
};
class NativeCallDuration {
public:
    // Measures local observation time while verified SIP remains InCall.
    // This does not establish media activity or remote-party confirmation.
    CallDurationSnapshot observe(const SipRuntimeObservation& observation,
                                 std::uint64_t now_ms) {
        if (observation.process != RuntimeProcess::Running ||
            observation.evidence != SipEvidence::VerifiedNativeApi ||
            observation.session == 0 || observation.call_id == 0 ||
            observation.sip != SipStatus::InCall) {
            reset();
            return {};
        }
        if (!active_ || session_ != observation.session || call_id_ != observation.call_id) {
            active_ = true;
            session_ = observation.session;
            call_id_ = observation.call_id;
            start_ms_ = now_ms;
            last_ms_ = now_ms;
        } else if (now_ms > last_ms_) {
            last_ms_ = now_ms;
        }
        // last_ms_ >= start_ms_; subtraction cannot wrap, even at UINT64_MAX.
        return {true, last_ms_ - start_ms_};
    }
    void reset() {
        active_ = false;
        session_ = 0;
        call_id_ = 0;
        start_ms_ = 0;
        last_ms_ = 0;
    }
private:
    bool active_ = false;
    std::uint64_t session_ = 0;
    std::uint64_t call_id_ = 0;
    std::uint64_t start_ms_ = 0;
    std::uint64_t last_ms_ = 0;
};
} // namespace axv
