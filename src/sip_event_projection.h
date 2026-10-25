#pragma once
#include "baresip_event_mailbox.h"
#include "baresip_event_identity.h"
#include <cstdint>
#include "legacy_sip_event_projection.h"

namespace axv {
// UI-thread projection of Baresip events. This class does NOT authenticate
// event provenance: the audited adapter must provide VerifiedNativeApi and
// the selected opaque account identity. Current production caller uses None.
class SipEventProjection {
public:
    const SipRuntimeObservation& observe(RuntimeProcess process, SipEvidence evidence,
        const BaresipEventMailbox::Batch& batch) {
        return legacy_.observe(process, evidence, batch);
    }
    const SipRuntimeObservation& observe(RuntimeProcess process, SipEvidence evidence,
                                         const BaresipEventMailbox::Batch& batch,
                                         std::uint64_t expected_account_token) {
        if (process != RuntimeProcess::Running || batch.session == 0) {
            if (bound_.observation.session != 0)
                rejected_session_ = bound_.observation.session;
            invalidate_identity(bound_);
            bound_.observation.process = process;
            return bound_.observation;
        }
        if (batch.session == rejected_session_) {
            invalidate_identity(bound_);
            bound_.observation.process = process;
            return bound_.observation;
        }
        if (batch.overflow || batch.size > batch.events.size()) {
            reject(batch.session,process);
            return bound_.observation;
        }
        if (evidence != SipEvidence::VerifiedNativeApi || expected_account_token == 0) {
            // Provenance can never revive within a previously verified generation.
            if (bound_.observation.evidence == SipEvidence::VerifiedNativeApi)
                rejected_session_ = batch.session;
            invalidate_identity(bound_);
            bound_.observation.process = process;
            return bound_.observation;
        }
        if (bound_.observation.session != batch.session ||
            bound_.observation.evidence != evidence) {
            invalidate_identity(bound_);
            bound_.observation = {process,SipStatus::Unavailable,evidence,batch.session};
            bound_.expected_account_token = expected_account_token;
        }
        if (bound_.expected_account_token != expected_account_token) {
            reject(batch.session,process); // Account switch requires a new session.
            return bound_.observation;
        }
        for (std::size_t i=0;i<batch.size;++i) {
            if (apply_identified_event(bound_,batch.session,batch.events[i])
                != IdentityResult::Applied) {
                reject(batch.session,process); // Reject ALL subsequent events this generation.
                break;
            }
        }
        return bound_.observation;
    }
    const SipRuntimeObservation& snapshot() const { return bound_.observation; }
private:
    void reject(std::uint64_t session, RuntimeProcess process) {
        rejected_session_=session;
        invalidate_identity(bound_);
        bound_.observation.process=process;
    }
    LegacySipEventProjection legacy_;
    BoundBaresipState bound_{};
    std::uint64_t rejected_session_=0;
};
} // namespace axv
