#include "../src/incoming_ringtone_policy.h"
#include "../src/diagnostic_export.h"
#include <cstdio>
#include <cstdlib>
#include <new>
#include <type_traits>

namespace {
bool allocations_forbidden = false;
std::size_t rejected_allocations = 0;
}

void* operator new(std::size_t size) {
 if (allocations_forbidden) {
  ++rejected_allocations;
  throw std::bad_alloc();
 }
 if (void* pointer = std::malloc(size ? size : 1)) return pointer;
 throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

// Checks remain enabled in Release builds.
#define CHECK(value) do { if (!(value)) { \
 std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #value); \
 std::abort(); } } while (false)

namespace {
void check_muted_without_allocation(const axv::IncomingRingtonePolicy& policy,
                                    const axv::SipRuntimeObservation& call,
                                    const std::wstring& endpoint) {
 const auto previous_rejections = rejected_allocations;
 bool escaped = false;
 bool empty = false;
 allocations_forbidden = true;
 try {
  const auto request = policy.request(call, endpoint, false, true);
  empty = !request.active() && !request.session && !request.call_id && request.endpoint_id.empty();
 } catch (...) { escaped = true; }
 allocations_forbidden = false;
 CHECK(!escaped);
 CHECK(empty);
 CHECK(rejected_allocations == previous_rejections);
}
}

int main() {
 using namespace axv;
 static_assert(std::is_same_v<decltype(CallReadiness::destination_syntax), bool>);
 static_assert(std::is_same_v<decltype(CallReadiness::account_syntax), bool>);
 static_assert(std::is_same_v<decltype(CallReadiness::native_engine), bool>);
 static_assert(std::is_same_v<decltype(CallReadiness::running_process), bool>);
 static_assert(std::is_same_v<decltype(CallReadiness::verified_registration), bool>);
 static_assert(std::is_same_v<decltype(CallReadiness::verified_media), bool>);
 static_assert(std::is_same_v<decltype(CallReadiness::dial_control_available), bool>);
 static_assert(std::is_same_v<decltype(NativeSecurityStatus::policy_selected), bool>);
 static_assert(std::is_same_v<decltype(NativeSecurityStatus::tls_required), bool>);
 static_assert(std::is_same_v<decltype(NativeSecurityStatus::running_process), bool>);
 static_assert(std::is_same_v<decltype(NativeSecurityStatus::verified_registration), bool>);

 const SipRuntimeObservation call{RuntimeProcess::Running, SipStatus::Ringing,
                                SipEvidence::VerifiedNativeApi, 1, 2, 2};
 // Build outside the failure window; this length requires an endpoint copy allocation.
 const std::wstring endpoint = L"private-device-endpoint-" + std::wstring(2048, L'x');
 const CallReadiness readiness{true, true, true, true, true, false, true};
 const auto security = native_security_status(true, true, call);
 const auto diagnostic_before = format_diagnostic_export(readiness, security);
 IncomingRingtonePolicy policy;
 check_muted_without_allocation(policy, call, endpoint);
 CHECK(policy.request(call, endpoint).endpoint_id == endpoint);

 // The existing unmuted API may throw on copying the endpoint. Its failure must
 // neither acknowledge this call nor change subsequent mute/unmute behavior.
 bool copy_failed = false;
 const auto previous_rejections = rejected_allocations;
 allocations_forbidden = true;
 try { (void)policy.request(call, endpoint); }
 catch (const std::bad_alloc&) { copy_failed = true; }
 allocations_forbidden = false;
 CHECK(copy_failed);
 CHECK(rejected_allocations == previous_rejections + 1);
 check_muted_without_allocation(policy, call, endpoint);
 const auto retry = policy.request(call, endpoint);
 CHECK(retry.active() && retry.session == call.session && retry.call_id == call.call_id);
 CHECK(retry.endpoint_id == endpoint);

 policy.silence(call);
 check_muted_without_allocation(policy, call, endpoint);
 CHECK(!policy.request(call, endpoint).active());
 auto next = call;
 ++next.call_id;
 check_muted_without_allocation(policy, next, endpoint);
 CHECK(policy.request(next, endpoint).active());
 // Muting a different call must preserve the acknowledgement for the first.
 CHECK(!policy.request(call, endpoint).active());
 next = call;
 ++next.session;
 check_muted_without_allocation(policy, next, endpoint);
 CHECK(policy.request(next, endpoint).active());
 CHECK(!policy.request(call, endpoint).active());

 const auto diagnostic_after = format_diagnostic_export(readiness, security);
 CHECK(diagnostic_after == diagnostic_before);
 CHECK(diagnostic_after.find(endpoint) == std::wstring::npos);
 CHECK(diagnostic_after.find(L"private-device-endpoint-") == std::wstring::npos);
 CHECK(diagnostic_after.find(L"sound_muted") == std::wstring::npos);
}
