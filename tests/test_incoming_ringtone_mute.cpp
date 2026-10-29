#include "../src/incoming_ringtone_policy.h"
#include "../src/native_session_control.h"
#include <cstdio>
#include <cstdlib>

// Remains active in Release builds where NDEBUG disables assert().
#define CHECK(condition) do { if (!(condition)) { \
 std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #condition); \
 std::abort(); } } while (0)

int main() {
 using namespace axv;
 const SipRuntimeObservation incoming{RuntimeProcess::Running, SipStatus::Ringing,
                                     SipEvidence::VerifiedNativeApi, 1, 2, 2};
 IncomingRingtonePolicy policy;
 const auto normal = policy.request(incoming, L"speaker");
 CHECK(normal.active());
 CHECK(normal.session == incoming.session && normal.call_id == incoming.call_id);
 CHECK(normal.endpoint_id == L"speaker");
 CHECK(policy.request(incoming, L"speaker", false, false).active());
 CHECK(!policy.request(incoming, L"speaker", false, true).active());
 // A temporary sound preference must not acknowledge this call.
 CHECK(policy.request(incoming, L"speaker", false, false).active());
 CHECK(!policy.request(incoming, L"speaker", true, false).active());
 CHECK(!policy.request(incoming, L"speaker", true, true).active());
 CHECK(policy.request(incoming, L"speaker").active());
 // Muting sound leaves the incoming observation and answer eligibility intact.
 CHECK(incoming.sip == SipStatus::Ringing);
 CHECK(native_displayed_call_command_current(NativeCallAction::Answer, incoming, incoming));

 policy.silence(incoming);
 CHECK(!policy.request(incoming, L"speaker", false, false).active());
 CHECK(!policy.request(incoming, L"speaker", false, true).active());
 CHECK(!policy.request(incoming, L"speaker").active());
 auto next = incoming;
 next.call_id = 3;
 CHECK(policy.request(next, L"speaker").active());
 CHECK(!policy.request(next, L"speaker", false, true).active());
 CHECK(policy.request(next, L"speaker").active());
 next = incoming;
 next.session = 2;
 CHECK(policy.request(next, L"speaker").active());
 CHECK(!policy.request(next, L"speaker", false, true).active());
 CHECK(policy.request(next, L"speaker").active());

 IncomingRingtonePolicy fresh;
 CHECK(fresh.request(incoming, L"speaker").active());
 auto invalid = incoming;
 invalid.sip = SipStatus::Calling;
 CHECK(!fresh.request(invalid, L"speaker", false, false).active());
 CHECK(!fresh.request(invalid, L"speaker", false, true).active());
 invalid = incoming; invalid.evidence = SipEvidence::None;
 CHECK(!fresh.request(invalid, L"speaker", false, false).active());
 invalid = incoming; invalid.process = RuntimeProcess::Stopping;
 CHECK(!fresh.request(invalid, L"speaker", false, false).active());
 invalid = incoming; invalid.session = 0;
 CHECK(!fresh.request(invalid, L"speaker", false, false).active());
 invalid = incoming; invalid.call_id = 0;
 CHECK(!fresh.request(invalid, L"speaker", false, false).active());
 fresh.silence(invalid);
 CHECK(fresh.request(incoming, L"speaker").active());

 CHECK(fresh.request(incoming, L"", false, false).active());
 CHECK(fresh.request(incoming, L"", false, false).endpoint_id.empty());
 CHECK(fresh.request(incoming, std::wstring(4096, L'x'), false, false).active());
 CHECK(!fresh.request(incoming, std::wstring(4097, L'x'), false, false).active());
 CHECK(!fresh.request(incoming, std::wstring(L"a\0b", 3), false, false).active());
 CHECK(!fresh.request(incoming, std::wstring(4096, L'x'), false, true).active());
 CHECK(fresh.request(incoming, L"speaker").active());
}
