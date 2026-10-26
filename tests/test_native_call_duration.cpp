#include "../src/native_call_duration.h"
#include <cstdio>
#include <limits>

using namespace axv;
namespace {
int failures = 0;
void expect(CallDurationSnapshot actual, bool active, std::uint64_t elapsed, const char* name) {
    if (actual.active != active || actual.elapsed_ms != elapsed) {
        std::fprintf(stderr, "%s: expected (%d, %llu), got (%d, %llu)\n", name,
            active, static_cast<unsigned long long>(elapsed), actual.active,
            static_cast<unsigned long long>(actual.elapsed_ms));
        ++failures;
    }
}
SipRuntimeObservation in_call() {
    SipRuntimeObservation o;
    o.process = RuntimeProcess::Running;
    o.evidence = SipEvidence::VerifiedNativeApi;
    o.sip = SipStatus::InCall;
    o.session = 1;
    o.call_id = 2;
    return o;
}
}
int main() {
    NativeCallDuration timer;
    auto o = in_call();
    expect(timer.observe(o, 0), true, 0, "zero is a valid start");
    expect(timer.observe(o, 1500), true, 1500, "same call advances");
    expect(timer.observe(o, 1000), true, 1500, "clock regression clamps");
    expect(timer.observe(o, 1600), true, 1600, "clock resumes from high water mark");
    o.call_id = 3;
    expect(timer.observe(o, 2000), true, 0, "new call restarts");
    expect(timer.observe(o, 2300), true, 300, "new call advances");
    o.session = 2;
    expect(timer.observe(o, 2400), true, 0, "new session restarts");
    timer.reset();
    expect(timer.observe(o, 2500), true, 0, "explicit reset restarts");

    // Every invalid observation discards the previous start, even if identity returns.
    for (int scenario = 0; scenario < 13; ++scenario) {
        timer.reset();
        o = in_call();
        expect(timer.observe(o, 100), true, 0, "valid before invalid observation");
        auto invalid = o;
        switch (scenario) {
        case 0: invalid.evidence = SipEvidence::None; break;
        case 1: invalid.session = 0; break;
        case 2: invalid.call_id = 0; break;
        case 3: invalid.process = RuntimeProcess::Stopped; break;
        case 4: invalid.process = RuntimeProcess::Stopping; break;
        case 5: invalid.process = RuntimeProcess::Failed; break;
        case 6: invalid.sip = SipStatus::Unavailable; break;
        case 7: invalid.sip = SipStatus::Disconnected; break;
        case 8: invalid.sip = SipStatus::Registering; break;
        case 9: invalid.sip = SipStatus::Registered; break;
        case 10: invalid.sip = SipStatus::Calling; break;
        case 11: invalid.sip = SipStatus::Ringing; break;
        case 12: invalid.sip = SipStatus::Failed; break;
        }
        expect(timer.observe(invalid, 500), false, 0, "invalid observation resets");
        expect(timer.observe(o, 600), true, 0, "return to same identity starts fresh");
    }
    timer.reset();
    o = in_call();
    const auto max = std::numeric_limits<std::uint64_t>::max();
    expect(timer.observe(o, 0), true, 0, "extreme interval begins at zero");
    expect(timer.observe(o, max), true, max, "full uint64 interval does not overflow");
    expect(timer.observe(o, 0), true, max, "wrap is a regressive clock");
    timer.reset();
    expect(timer.observe(o, max - 5), true, 0, "start near maximum");
    expect(timer.observe(o, max), true, 5, "small interval near maximum");
    timer.reset();
    expect(timer.observe(o, max), true, 0, "maximum is a valid start");
    expect(timer.observe(o, 0), true, 0, "regression at maximum stays zero");
    return failures == 0 ? 0 : 1;
}
