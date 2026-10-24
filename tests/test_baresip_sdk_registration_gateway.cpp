#include "../src/baresip_sdk_registration_gateway.h"
#include "../src/sip_event_projection.h"
#include <cstdint>
#include <atomic>
#include <thread>

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

    // A registration that stays PENDING must not become a usable session.
    if (gateway.submit(&selected)!=NativeRegistrationSubmission::Submitted) return 27;
    if (gateway.arm_registration_deadline(1000,0) ||
        !gateway.arm_registration_deadline(1000,200) ||
        gateway.arm_registration_deadline(1001,200)) return 28;
    if (gateway.poll_registration_deadline(1000)!=RegistrationDeadlineVerdict::Pending ||
        gateway.poll_registration_deadline(1199)!=RegistrationDeadlineVerdict::Pending)
        return 29;
    if (gateway.poll_registration_deadline(1200)!=RegistrationDeadlineVerdict::Expired)
        return 30;
    if (callback || gateway.session() || mailbox.drain().session ||
        unregistrations!=4 || callback_added!=callback_removed ||
        gateway.poll_registration_deadline(1201)!=RegistrationDeadlineVerdict::Unarmed)
        return 31;

    // Timely callback wins, but only while the SDK itself reports registration.
    if (gateway.submit(&selected)!=NativeRegistrationSubmission::Submitted ||
        !gateway.arm_registration_deadline(2000,300)) return 32;
    bevent timely{&selected,nullptr};
    g_selected_registered=true;
    callback(BEVENT_REGISTER_OK,&timely,context);
    if (gateway.poll_registration_deadline(2200)!=
        RegistrationDeadlineVerdict::CallbackConfirmed ||
        !gateway.submitted() ||
        gateway.poll_registration_deadline(2400)!=RegistrationDeadlineVerdict::Unarmed)
        return 33;
    gateway.stop();
    if (callback || unregistrations!=5) return 34;

    // Clock rollback must abort, never extend an in-flight submission.
    if (gateway.submit(&selected)!=NativeRegistrationSubmission::Submitted ||
        !gateway.arm_registration_deadline(5000,1000)) return 35;
    if (gateway.poll_registration_deadline(4999)!=RegistrationDeadlineVerdict::ClockRegressed ||
        gateway.session() || callback || unregistrations!=6) return 36;

    // An old deadline cannot survive a revoked generation.
    if (gateway.submit(&selected)!=NativeRegistrationSubmission::Submitted ||
        !gateway.arm_registration_deadline(6000,300)) return 37;
    bevent shutdown{&selected,nullptr};
    callback(BEVENT_SHUTDOWN,&shutdown,context);
    if (gateway.poll_registration_deadline(6010)!=RegistrationDeadlineVerdict::SessionRevoked ||
        callback || gateway.session() || unregistrations!=6) return 38;
    if (gateway.submit(&selected)!=NativeRegistrationSubmission::Submitted ||
        gateway.poll_registration_deadline(6500)!=RegistrationDeadlineVerdict::Unarmed)
        return 39;
    gateway.stop();
    if (callback || unregistrations!=7 ||
        callback_added!=callback_removed) return 40;

    // Thread ownership test: worker must not invoke ANY native SDK API.
    // There is a live callback and pending deadline to make unauthorized
    // cleanup observable without a data race in fake-SDK globals.
    if (gateway.submit(&selected) != NativeRegistrationSubmission::Submitted ||
        !gateway.arm_registration_deadline(7000,200)) return 41;
    const auto session_before = gateway.session();
    const auto register_before = submissions;
    const auto unregister_before = unregistrations;
    std::atomic<int> rejected{0};
    std::thread foreign([&] {
        if (gateway.submit(&another) ==
            NativeRegistrationSubmission::WrongEventLoopThread) ++rejected;
        if (!gateway.stop()) ++rejected;
        if (!gateway.arm_registration_deadline(7100,100)) ++rejected;
        if (gateway.poll_registration_deadline(7300) ==
            RegistrationDeadlineVerdict::WrongEventLoopThread) ++rejected;
        if (!gateway.submitted() && gateway.session() == 0) ++rejected;
    });
    foreign.join();
    if (rejected != 5 || submissions != register_before ||
        unregistrations != unregister_before ||
        gateway.session() != session_before ||
        gateway.poll_registration_deadline(7199) != RegistrationDeadlineVerdict::Pending)
        return 42;
    if (!gateway.stop() || gateway.session() || callback ||
        unregistrations != unregister_before + 1 ||
        callback_added != callback_removed) return 43;

    // Even an idle gateway cannot be started from a foreign thread.
    std::atomic<bool> foreign_start_blocked{false};
    std::thread foreign_start([&] {
        foreign_start_blocked =
            gateway.submit(&selected) == NativeRegistrationSubmission::WrongEventLoopThread;
    });
    foreign_start.join();
    if (!foreign_start_blocked || submissions != register_before) return 44;
    return 0;
}
