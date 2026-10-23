#include "../src/baresip_sdk_registration_gateway.h"
#include "../src/sip_event_projection.h"
#include <cstdint>

static bevent_h* callback = nullptr;
static void* context = nullptr;
static bool callback_failure = false;
static bool submission_failure = false;
// Test-only forced callback during ua_register(): 1=UA shutdown,
// 2=global exit, 3=fake success without native registrar confirmation.
static unsigned terminal_during_submission = 0;
static unsigned callback_added = 0, callback_removed = 0;
static unsigned submissions = 0, unregistrations = 0;
static bool g_selected_registered = false;

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
extern "C" bool ua_isregistered(const ua* selected) {
    return selected && selected->test_id == 1 && g_selected_registered;
}
extern "C" int ua_register(ua* selected) {
    if (!selected) return 18;
    ++submissions;
    g_selected_registered = false; // Submission does not register.
    bevent event{selected,nullptr};
    if (callback) callback(BEVENT_REGISTERING,&event,context);
    if (callback && terminal_during_submission == 1)
        callback(BEVENT_SHUTDOWN,&event,context);
    if (callback && terminal_during_submission == 2)
        callback(BEVENT_EXIT,nullptr,context);
    if (callback && terminal_during_submission == 3)
        callback(BEVENT_REGISTER_OK,&event,context);
    return submission_failure ? 19 : 0;
}
extern "C" void ua_unregister(ua* selected) {
    if (!selected) return;
    ++unregistrations;
    g_selected_registered = false;
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
    g_selected_registered = true; // Fake registrar accepted credentials.
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

    // A forged/stale REGISTER_OK cannot register a UA still marked down
    // by the SDK itself. The entire bridge generation is invalidated.
    if (gateway.submit(&selected) != NativeRegistrationSubmission::Submitted) return 15;
    const auto pending = mailbox.drain();
    if (pending.size != 1 || pending.events[0].kind != BaresipEventKind::Registering ||
        ua_isregistered(&selected)) return 16;
    bevent unconfirmed{&selected,nullptr};
    callback(BEVENT_REGISTER_OK,&unconfirmed,context);
    if (adapter.active() || gateway.submitted() || gateway.session() ||
        mailbox.drain().session != 0) return 17;
    // A revoked native UA may already be destroyed. stop() must not
    // attempt ua_unregister() when the bridge is no longer live.
    gateway.stop();
    if (callback || callback_added != callback_removed ||
        unregistrations != 2) return 18;

    // A terminal event DURING a successful ua_register() call must not
    // be reported as an active registration submission.
    for (unsigned terminal = 1; terminal <= 3; ++terminal) {
        terminal_during_submission = terminal;
        if (gateway.submit(&selected) !=
                NativeRegistrationSubmission::NativeSessionRevoked ||
            adapter.active() || gateway.submitted() || gateway.session() ||
            mailbox.drain().session || callback ||
            callback_added != callback_removed || unregistrations != 2)
            return 18 + static_cast<int>(terminal);
    }
    terminal_during_submission = 0;

    // An asynchronous shutdown after a normal submit requires cleanup
    // before another account can start. Cleanup must not touch the dead UA.
    if (gateway.submit(&selected) != NativeRegistrationSubmission::Submitted)
        return 22;
    bevent stopping{&selected,nullptr};
    callback(BEVENT_SHUTDOWN,&stopping,context);
    if (adapter.active() || gateway.submitted() ||
        gateway.submit(&another) != NativeRegistrationSubmission::NativeSessionRevoked)
        return 23;
    gateway.stop();
    if (callback || gateway.session() || unregistrations != 2)
        return 24;

    // Fresh generation can recover after explicit cleanup.
    if (gateway.submit(&selected) != NativeRegistrationSubmission::Submitted)
        return 25;
    gateway.stop();
    if (callback || unregistrations != 3 ||
        callback_added != callback_removed) return 26;
    return 0;
}
