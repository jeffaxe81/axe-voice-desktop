// Synthetic, network-free command/callback security regression. Checks stay on
// in Release: intentionally do not use assert().
#include "../src/native_session_control.h"
#include "../src/baresip_callback_lifecycle.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); std::exit(1); } } while (0)

static void stale_command_matrix() {
    using namespace axv;
    const NativeCallAction controls[] = {NativeCallAction::Answer, NativeCallAction::Hangup,
        NativeCallAction::SetMute, NativeCallAction::SetHold, NativeCallAction::Transfer,
        NativeCallAction::Dtmf, NativeCallAction::Dial};
    for (auto action : controls) {
        SipRuntimeObservation live{RuntimeProcess::Running, SipStatus::InCall,
            SipEvidence::VerifiedNativeApi, 42, 7, 7};
        if (action == NativeCallAction::Answer) live.sip = SipStatus::Ringing;
        if (action == NativeCallAction::Dial) { live.sip = SipStatus::Registered; live.call_id = 0; }
        NativeCallCommand command{action, live.session, live.call_id, "5"};
        CHECK(native_command_is_current(command, live));
        auto stale = command;
        for (auto session : {std::uint64_t(0), std::uint64_t(41), std::uint64_t(43),
                             std::numeric_limits<std::uint64_t>::max()}) {
            stale = command; stale.session = session;
            CHECK(!native_command_is_current(stale, live));
        }
        stale = command; stale.call_id = command.call_id + 1;
        CHECK(!native_command_is_current(stale, live));
        for (auto process : {RuntimeProcess::Stopped, RuntimeProcess::Stopping, RuntimeProcess::Failed}) {
            auto revoked = live; revoked.process = process;
            CHECK(!native_command_is_current(command, revoked));
        }
        auto unverified = live; unverified.evidence = SipEvidence::None;
        CHECK(!native_command_is_current(command, unverified));
        auto zero_epoch = live; zero_epoch.session = 0; stale = command; stale.session = 0;
        CHECK(!native_command_is_current(stale, zero_epoch));
    }
}

static void displayed_call_and_media_identity() {
    using namespace axv;
    SipRuntimeObservation live{RuntimeProcess::Running, SipStatus::Ringing,
        SipEvidence::VerifiedNativeApi, 13, 9, 9};
    for (auto action : {NativeCallAction::Answer, NativeCallAction::Hangup}) {
        CHECK(native_displayed_call_command_current(action, live, live));
        auto replacement = live; ++replacement.call_id; ++replacement.last_call_id;
        CHECK(!native_displayed_call_command_current(action, live, replacement));
        replacement = live; ++replacement.session;
        CHECK(!native_displayed_call_command_current(action, live, replacement));
        replacement = live; replacement.process = RuntimeProcess::Stopping;
        CHECK(!native_displayed_call_command_current(action, live, replacement));
    }
    live.sip = SipStatus::InCall;
    NativeCallMediaState media{13,9,true,false,false};
    CHECK(native_dtmf_controls_current(media, live));
    NativeDtmfMode mode; CHECK(mode.toggle(media, live)); CHECK(mode.active(media, live));
    auto replacement = live; ++replacement.call_id;
    mode.synchronize(media, replacement); CHECK(!mode.armed());
    media.local_hold = true; CHECK(!native_dtmf_controls_current(media, live));
    media.local_hold = false;
    for (auto transfer : {NativeTransferState::Submitted, NativeTransferState::Failed}) {
        media.transfer = transfer; CHECK(!native_dtmf_controls_current(media, live));
    }
    NativeCallCommand digit{NativeCallAction::Dtmf,13,9,{}};
    for (unsigned key = 0; key < 256; ++key) {
        digit.destination.assign(1, static_cast<char>(key));
        const bool expected = (key >= '0' && key <= '9') || key == '*' || key == '#';
        CHECK(native_command_is_current(digit, live) == expected);
    }
    digit.destination = "12"; CHECK(!native_command_is_current(digit, live));
    digit.destination.clear(); CHECK(!native_command_is_current(digit, live));
}

static void late_callbacks_cannot_reauthorize_commands() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle(mailbox);
    const auto first = lifecycle.begin(123);
    CHECK(first);
    CHECK(lifecycle.registration(first, BaresipEventKind::Registering));
    CHECK(lifecycle.registration(first, BaresipEventKind::RegisterOk));
    std::uint64_t first_token = 0;
    CHECK(lifecycle.call_started(first, 456, BaresipEventKind::CallIncoming, &first_token));
    CHECK(first_token != 0);
    lifecycle.revoke_and_wait();
    CHECK(mailbox.drain().size == 0);
    CHECK(!lifecycle.registration(first, BaresipEventKind::RegisterOk));
    CHECK(!lifecycle.call_changed(first, 456, BaresipEventKind::CallEstablished));
    const auto second = lifecycle.begin(123); CHECK(second);
    CHECK(second.mailbox_session != first.mailbox_session);
    CHECK(!lifecycle.hold_native_callback(first));
    CHECK(!lifecycle.call_started(first, 789, BaresipEventKind::CallIncoming));
    CHECK(!lifecycle.call_changed(first, 456, BaresipEventKind::CallClosed));
    CHECK(lifecycle.registration(second, BaresipEventKind::Registering));
    CHECK(lifecycle.registration(second, BaresipEventKind::RegisterOk));
    std::uint64_t second_token = 0;
    CHECK(lifecycle.call_started(second, 789, BaresipEventKind::CallIncoming, &second_token));
    SipEventProjection projection;
    const auto live = project_native_sdk_batch(projection, mailbox.drain(), true);
    CHECK(live.sip == SipStatus::Ringing);
    CHECK(native_command_is_current({NativeCallAction::Answer, second.mailbox_session, second_token, {}}, live));
    // Tokens may restart at one; generation is an independent required fence.
    CHECK(!native_command_is_current({NativeCallAction::Hangup, first.mailbox_session, first_token, {}}, live));
    lifecycle.reject_current();
    CHECK(!lifecycle.call_changed(second, 789, BaresipEventKind::CallEstablished));
    CHECK(!lifecycle.registration(second, BaresipEventKind::RegisterOk));
    lifecycle.revoke_and_wait();
}

int main() {
    stale_command_matrix();
    displayed_call_and_media_identity();
    late_callbacks_cannot_reauthorize_commands();
    return 0;
}
