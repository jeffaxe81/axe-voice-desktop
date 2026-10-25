#pragma once
#include "baresip_event_mailbox.h"
#include <cstdint>

namespace axv {
// UI-thread projection of an authenticated native event stream.
// This class does not authenticate producers. Process/log probes MUST use None.
// VerifiedNativeApi is reserved for a separately audited in-process adapter.
class LegacySipEventProjection {
public:
    const SipRuntimeObservation& observe(RuntimeProcess process, SipEvidence evidence,
                                         const BaresipEventMailbox::Batch& batch) {
        if (process != RuntimeProcess::Running || batch.session == 0) {
            if (state_.session != 0)
                rejected_session_ = state_.session;
            state_ = {process, SipStatus::Unavailable, SipEvidence::None, 0};
            return state_;
        }
        if (batch.session == rejected_session_) {
            state_ = {process, SipStatus::Unavailable, SipEvidence::None, 0};
            return state_;
        }
        if (batch.overflow) {
            reject(batch.session, process);
            return state_;
        }
        if (evidence != SipEvidence::VerifiedNativeApi) {
            // If provenance is revoked, this generation cannot be trusted again.
            if (state_.evidence == SipEvidence::VerifiedNativeApi)
                rejected_session_ = batch.session;
            state_ = {process, SipStatus::Unavailable, SipEvidence::None, 0};
            return state_;
        }
        if (state_.session != batch.session || state_.evidence != evidence) {
            // No SIP state is inherited across generations.
            state_ = {process, SipStatus::Unavailable, evidence, batch.session};
        }
        for (std::size_t i = 0; i < batch.size; ++i) {
            if (apply_baresip_event(state_, batch.session, batch.events[i])
                != EventTransition::Applied) {
                reject(batch.session, process); // fail closed on a broken sequence
                break;
            }
        }
        return state_;
    }
    const SipRuntimeObservation& snapshot() const { return state_; }
private:
    void reject(std::uint64_t session, RuntimeProcess process) {
        rejected_session_ = session; // cannot recover until a NEW generation
        state_ = {process, SipStatus::Unavailable, SipEvidence::None, 0};
    }
    SipRuntimeObservation state_{};
    std::uint64_t rejected_session_ = 0;
};
} // namespace axv
