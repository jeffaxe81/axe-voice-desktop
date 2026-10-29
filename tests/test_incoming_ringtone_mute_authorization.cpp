#include "../src/incoming_ringtone_policy.h"
#include "../src/native_session_control.h"
#include <cstdlib>
#include <string>
#define CHECK(x) do { if (!(x)) std::abort(); } while (0)

namespace {
void same_observation(const axv::SipRuntimeObservation& a,
                      const axv::SipRuntimeObservation& b) {
 CHECK(a.process == b.process);
 CHECK(a.sip == b.sip);
 CHECK(a.evidence == b.evidence);
 CHECK(a.session == b.session);
 CHECK(a.call_id == b.call_id);
 CHECK(a.last_call_id == b.last_call_id);
}
void check_controls(const axv::SipRuntimeObservation& displayed,
                    const axv::SipRuntimeObservation& current, bool expected) {
 for (auto action : {axv::NativeCallAction::Answer, axv::NativeCallAction::Hangup})
  CHECK(axv::native_displayed_call_command_current(action, displayed, current) == expected);
}
}
int main() {
 // Synthetic observations exercise authorization rules; they do not establish
 // that a native engine/account is registered or that an SDK action occurred.
 const axv::SipRuntimeObservation incoming{
  axv::RuntimeProcess::Running, axv::SipStatus::Ringing,
  axv::SipEvidence::VerifiedNativeApi, 17, 29, 29};
 auto call = incoming;
 axv::IncomingRingtonePolicy policy;
 const auto banner = std::wstring(axv::runtime_status_banner(call, 17));
 for (bool muted : {false, true, true, false}) {
  const auto request = policy.request(call, L"ring-output", false, muted);
  CHECK(request.active() == !muted);
  if (!muted) {
   CHECK(request.session == 17 && request.call_id == 29);
   CHECK(request.endpoint_id == L"ring-output");
  }
  same_observation(call, incoming);
  CHECK(axv::verified_incoming_call(call));
  CHECK(axv::sip_proof_is_current(call, 17));
  CHECK(axv::effective_sip_status(call, 17) == axv::SipStatus::Ringing);
  CHECK(std::wstring(axv::runtime_status_banner(call, 17)) == banner);
  check_controls(call, call, true);
 }
 // Muting is sound-only: it neither consumes Answer/Hangup admission nor
 // changes the observation. This is a policy test, not an SDK dispatch test.
 axv::NativeCommandAdmission admission;
 CHECK(!policy.request(call, L"ring-output", false, true).active());
 for (auto action : {axv::NativeCallAction::Answer, axv::NativeCallAction::Hangup}) {
  const axv::NativeCallCommand command{action, call.session, call.call_id, {}};
  CHECK(admission.admit(command, call));
  CHECK(!admission.admit(command, call));
 }
 same_observation(call, incoming);
 CHECK(policy.request(call, L"ring-output").active());
 CHECK(!policy.request(call, L"ring-output", true, false).active());
 CHECK(!policy.request(call, L"ring-output", true, true).active());
 CHECK(policy.request(call, L"ring-output", false, false).active());

 auto mismatch = call;
 mismatch.call_id++;
 check_controls(call, mismatch, false);
 CHECK(axv::verified_incoming_call(mismatch));
 mismatch = call; mismatch.session++;
 check_controls(call, mismatch, false);
 CHECK(!axv::sip_proof_is_current(mismatch, call.session));
 CHECK(axv::effective_sip_status(mismatch, call.session) == axv::SipStatus::Unavailable);

 for (auto process : {axv::RuntimeProcess::Stopped, axv::RuntimeProcess::Stopping,
                      axv::RuntimeProcess::Failed}) {
  mismatch = call; mismatch.process = process;
  const auto before = mismatch;
  CHECK(!policy.request(mismatch, L"ring-output", false, false).active());
  CHECK(!policy.request(mismatch, L"ring-output", false, true).active());
  CHECK(!axv::verified_incoming_call(mismatch));
  check_controls(call, mismatch, false);
  same_observation(mismatch, before);
 }
 mismatch = call; mismatch.evidence = axv::SipEvidence::None;
 CHECK(!policy.request(mismatch, L"ring-output", false, false).active());
 CHECK(!policy.request(mismatch, L"ring-output", false, true).active());
 CHECK(!axv::verified_incoming_call(mismatch));
 check_controls(call, mismatch, false);
 mismatch = call; mismatch.session = 0;
 CHECK(!policy.request(mismatch, L"ring-output", false, false).active());
 check_controls(call, mismatch, false);
 mismatch = call; mismatch.call_id = 0;
 CHECK(!policy.request(mismatch, L"ring-output", false, false).active());
 CHECK(!axv::verified_incoming_call(mismatch));
 check_controls(call, mismatch, false);
 for (auto status : {axv::SipStatus::Unavailable, axv::SipStatus::Disconnected,
                     axv::SipStatus::Registering, axv::SipStatus::Registered,
                     axv::SipStatus::Calling, axv::SipStatus::InCall,
                     axv::SipStatus::Failed}) {
  mismatch = call; mismatch.sip = status;
  CHECK(!policy.request(mismatch, L"ring-output", false, false).active());
  CHECK(!policy.request(mismatch, L"ring-output", false, true).active());
  CHECK(!axv::verified_incoming_call(mismatch));
  CHECK(!axv::native_displayed_call_command_current(axv::NativeCallAction::Answer, call, mismatch));
 }
 // Re-enabling sound must not undo per-call silence from an explicit action.
 policy.silence(call);
 CHECK(!policy.request(call, L"ring-output", false, true).active());
 CHECK(!policy.request(call, L"ring-output", false, false).active());
 same_observation(call, incoming);
 check_controls(call, call, true);
 call.call_id++;
 CHECK(!policy.request(call, L"ring-output", false, true).active());
 CHECK(policy.request(call, L"ring-output", false, false).active());
}
