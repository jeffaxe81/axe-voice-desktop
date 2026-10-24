#pragma once
// Optional SDK-LAB runtime controller. NEVER linked into AXEVoice.exe.
// All public methods must be serialized on one non-owner control thread.
#include "baresip_sdk_registration_runtime.h"
#include "native_registration_retry_budget.h"
#include <cstdint>
#include <memory>

namespace axv {
enum class NativeControlDecision {
    Allowed, NotConfigured, NoLiveGeneration, AlreadyPending,
    RetryDelayed, RetryExhausted, ClockFault, StartFailed, SubmissionRejected,
    CancelRejected, StopFailed, NativePending,
    CallbackObservedUnverified, TerminalNativeFailure, UserCancelled,
    CancellationRequested
};
struct NativeControlView {
    bool control_thread_running = false;
    bool account_provisioned = false;
    bool register_request_pending = false;
    bool cancellation_requested = false;
    bool terminal_failure_seen = false;
    bool callback_unverified_seen = false;
    bool restart_required = false;
    bool sip_authenticated = false; // explicitly NEVER inferred from a callback
    bool media_verified = false;
    std::uint64_t generation = 0;
    std::uint64_t next_attempt_ms = 0;
    std::uint32_t failures = 0;
    RegistrationDispatchKind last_dispatch = RegistrationDispatchKind::Idle;
};

// A host wrapper that actually connects the existing start/submit/cancel/stop
// APIs and limits attempt frequency. It does NOT implement SIP auth or media.
class ExperimentalNativeRegistrationController {
public:
    explicit ExperimentalNativeRegistrationController(
        std::unique_ptr<BaresipNativeRegistrationHost> host)
      : host_(std::move(host)) {}
    ExperimentalNativeRegistrationController(
        const ExperimentalNativeRegistrationController&) = delete;
    ExperimentalNativeRegistrationController& operator=(
        const ExperimentalNativeRegistrationController&) = delete;
    ~ExperimentalNativeRegistrationController() { (void)stop(); }

    NativeControlDecision start();
    NativeControlDecision request_registration(std::uint64_t monotonic_ms,
                                               std::uint32_t timeout_ms);
    NativeControlDecision cancel();
    // Only this explicit client poll reconciles terminal native dispatch and
    // accounts for a failure in the local retry budget (once per generation).
    NativeControlDecision poll_control_plane(std::uint64_t monotonic_ms);
    NativeControlDecision stop();
    void reset_retry_budget_with_explicit_consent();
    NativeControlView snapshot() const;

private:
    std::unique_ptr<BaresipNativeRegistrationHost> host_;
    NativeRegistrationRetryBudget retry_{};
    std::uint64_t generation_ = 0;
    bool pending_ = false;
    bool started_ = false;
    bool terminal_observed_ = false;
    bool callback_unverified_seen_ = false;
    bool cancel_requested_ = false;
};
} // namespace axv
