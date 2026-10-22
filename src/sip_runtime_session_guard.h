#pragma once
#include "sip_event_projection.h"

namespace axv {
// UI thread only. A new/failed/stopped process cannot inherit a former SIP
// registration. This guard does not authenticate event producers.
class SipRuntimeSessionGuard {
public:
    const SipRuntimeObservation& observe(RuntimeProcess process, SipEvidence evidence,
                                         BaresipEventMailbox& mailbox,
                                         SipEventProjection& projection) {
        // The first Running tick fences events queued before process admission.
        if (process != RuntimeProcess::Running || previous_ != RuntimeProcess::Running)
            mailbox.invalidate_session();
        previous_ = process;
        return projection.observe(process, evidence, mailbox.drain());
    }
    void revoke(RuntimeProcess process, BaresipEventMailbox& mailbox,
                SipEventProjection& projection) {
        if (process == RuntimeProcess::Running) process = RuntimeProcess::Stopped;
        mailbox.invalidate_session();
        previous_ = process;
        projection.observe(process, SipEvidence::None, mailbox.drain());
    }
private:
    RuntimeProcess previous_ = RuntimeProcess::Stopped;
};
} // namespace axv
