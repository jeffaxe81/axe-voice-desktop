#include "../src/baresip_sdk_registration_gateway.h"
#include "../src/sip_event_projection.h"
#include <cstdint>

static bevent_h* callback = nullptr;
static void* context = nullptr;
static bool callback_failure = false;
static bool submission_failure = false;
static unsigned callback_added = 0, callback_removed = 0;
static unsigned submissions = 0, unregistrations = 0;

extern "C" int bevent_register(bevent_h* fn,void* ctx) {
    if (callback_failure || callback) return 17;
    callback = fn; context = ctx; ++callback_added; return 0;
}
extern "C" void bevent_unregister(bevent_h* fn) {
    if (callback == fn) {
        callback = nullptr; context = nullptr; ++callback_removed;
    }
}
extern "C" ua* bevent_get_ua(const bevent* event) {
    return event ? event->owner : nullptr;
}
extern "C" call* bevent_get_call(const bevent* event) {
    return event ? event->active_call : nullptr;
}
extern "C" int ua_register(ua* selected) {
    if (!selected) return 18;
    ++submissions;
    bevent event{selected,nullptr};
    if (callback) callback(BEVENT_REGISTERING,&event,context);
    return submission_failure ? 19 : 0;
}
extern "C" void ua_unregister(ua* selected) {
    if (!selected) return;
    ++unregistrations;
    bevent event{selected,nullptr};
    if (callback) callback(BEVENT_UNREGISTERING,&event,context);
}

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle(mailbox);
    BaresipSdkEventAdapter adapter(lifecycle);
    BaresipSdkRegistrationGateway gateway(adapter);
    ua selected{1}, another{2};

    if (gateway.submit(nullptr)!=NativeRegistrationSubmission::InvalidAccount ||
        gateway.session() || submissions) return 1;
    callback_failure = true;
    if (gateway.submit(&selected)!=NativeRegistrationSubmission::BridgeUnavailable ||
        submissions || mailbox.drain().session) return 2;
    callback_failure = false;

    submission_failure = true;
    if (gateway.submit(&selected)!=NativeRegistrationSubmission::SubmissionRejected ||
        gateway.submitted() || mailbox.drain().session || callback ||
        callback_added != callback_removed) return 3;
    submission_failure = false;

    if (gateway.submit(&selected)!=NativeRegistrationSubmission::Submitted ||
        !gateway.submitted() || gateway.session()==0) return 4;
    const auto first = gateway.session();
    if (gateway.submit(&another)!=NativeRegistrationSubmission::AlreadySubmitted ||
        submissions!=2) return 5;

    SipEventProjection projection;
    auto batch = mailbox.drain();
    if (batch.session!=first || batch.size!=1 ||
        batch.events[0].kind!=BaresipEventKind::Registering) return 6;
    // Synthetic proof for state-machine tests only; not granted by gateway.
    auto state = projection.observe(RuntimeProcess::Running,
                                    SipEvidence::VerifiedNativeApi,batch);
    if (state.sip!=SipStatus::Registering ||
        may_use_call_control(state,first,CallAction::Dial)) return 7;

    bevent registered{&selected,nullptr};
    if (!callback) return 8;
    callback(BEVENT_REGISTER_OK,&registered,context);
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi,mailbox.drain());
    if (state.sip!=SipStatus::Registered) return 9; // Only native fake event.
    gateway.stop();
    if (gateway.submitted() || gateway.session() || mailbox.drain().session ||
        callback || unregistrations!=1 || callback_added!=callback_removed) return 10;

    if (gateway.submit(&another)!=NativeRegistrationSubmission::Submitted ||
        gateway.session()<=first) return 11;
    const auto later = mailbox.drain();
    if (later.size!=1 || later.events[0].kind!=BaresipEventKind::Registering) return 12;
    gateway.stop();
    if (callback || unregistrations!=2 || submissions!=3 ||
        callback_added!=callback_removed) return 13;
    if (mailbox.enqueue({BaresipEventKind::RegisterOk,first})) return 14;
    return 0;
}
