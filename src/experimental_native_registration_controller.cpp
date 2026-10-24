#include "experimental_native_registration_controller.h"

namespace axv {
NativeControlDecision ExperimentalNativeRegistrationController::start() {
    if (!host_) return NativeControlDecision::NotConfigured;
    if (started_) return NativeControlDecision::AlreadyPending;
    // No side effects on the user-visible Windows executable.
    if (!host_->start()) {
        (void)host_->stop();
        return NativeControlDecision::StartFailed;
    }
    generation_ = host_->generation();
    const auto native = host_->snapshot();
    if (!generation_ || !native.running || !native.account_provisioned) {
        (void)host_->stop();
        generation_ = 0;
        return NativeControlDecision::StartFailed;
    }
    started_ = true;
    pending_ = false;
    terminal_observed_ = false;
    callback_unverified_seen_ = false;
    cancel_requested_ = false;
    return NativeControlDecision::Allowed;
}
NativeControlDecision ExperimentalNativeRegistrationController::request_registration(
    std::uint64_t monotonic_ms, std::uint32_t timeout_ms) {
    if (!host_) return NativeControlDecision::NotConfigured;
    if (!started_ || !generation_ || generation_ != host_->generation())
        return NativeControlDecision::NoLiveGeneration;
    if (pending_) {
        const auto current = poll_control_plane(monotonic_ms);
        return current == NativeControlDecision::NativePending
            ? NativeControlDecision::AlreadyPending : current;
    }
    const auto admission = retry_.check(monotonic_ms);
    if (admission == NativeRetryAdmission::Exhausted)
        return NativeControlDecision::RetryExhausted;
    if (admission == NativeRetryAdmission::TimeRegressed)
        return NativeControlDecision::ClockFault;
    if (admission == NativeRetryAdmission::WaitForDeadline)
        return NativeControlDecision::RetryDelayed;
    if (!host_->submit_for_generation(generation_, timeout_ms)) {
        retry_.record_failed_attempt(monotonic_ms);
        (void)host_->cancel(); // Ensure an ambiguous submission cannot later complete.
        pending_ = true;       // Explicit stop/restart required.
        terminal_observed_ = true; // Never charge a second failure on a later poll.
        return NativeControlDecision::SubmissionRejected;
    }
    pending_ = true; // Submitted != authenticated, even on SDK success.
    return NativeControlDecision::Allowed;
}
NativeControlDecision ExperimentalNativeRegistrationController::cancel() {
    if (!host_) return NativeControlDecision::NotConfigured;
    if (!started_ || !generation_) return NativeControlDecision::NoLiveGeneration;
    if (!pending_) return NativeControlDecision::NoLiveGeneration;
    if (cancel_requested_) return NativeControlDecision::CancellationRequested;
    // An accepted cancellation is only queued, never described as completed.
    const bool ok = host_->cancel();
    if (ok) cancel_requested_ = true;
    return ok ? NativeControlDecision::CancellationRequested :
                NativeControlDecision::CancelRejected;
}

NativeControlDecision ExperimentalNativeRegistrationController::poll_control_plane(
    std::uint64_t monotonic_ms) {
    if (!host_) return NativeControlDecision::NotConfigured;
    if (!started_ || !generation_) return NativeControlDecision::NoLiveGeneration;
    const auto native = host_->snapshot();
    if (native.command_generation != generation_ ||
        generation_ != host_->generation() ||
        !native.running || native.reactor_fault != LibreReactorFault::None) {
        // A requested cancel is never charged as a failed login attempt.
        if (cancel_requested_) return NativeControlDecision::CancellationRequested;
        if (pending_ && !terminal_observed_) {
            retry_.record_failed_attempt(monotonic_ms);
            terminal_observed_ = true;
        }
        return NativeControlDecision::TerminalNativeFailure;
    }
    if (!pending_) return NativeControlDecision::NoLiveGeneration;
    const auto kind = native.last_dispatch;
    if (kind == RegistrationDispatchKind::Cancelled)
        return NativeControlDecision::UserCancelled;
    if (cancel_requested_) return NativeControlDecision::CancellationRequested;
    // Once a submission was conclusively failed, stale callbacks cannot
    // resurrect its status within the same generation.
    if (terminal_observed_) return NativeControlDecision::TerminalNativeFailure;
    if (kind == RegistrationDispatchKind::Confirmed) {
        callback_unverified_seen_ = true;
        return NativeControlDecision::CallbackObservedUnverified;
    }
    const bool failed = kind == RegistrationDispatchKind::RegistrarRejected ||
        kind == RegistrationDispatchKind::DeadlineExpired ||
        kind == RegistrationDispatchKind::Failed ||
        kind == RegistrationDispatchKind::OverflowStopped ||
        kind == RegistrationDispatchKind::WrongOwnerThread ||
        kind == RegistrationDispatchKind::SupersededDuringDispatch ||
        kind == RegistrationDispatchKind::StaleGeneration;
    if (failed) {
        if (!terminal_observed_) {
            retry_.record_failed_attempt(monotonic_ms);
            terminal_observed_ = true;
        }
        return NativeControlDecision::TerminalNativeFailure;
    }
    // Submitted / Pending / Idle are NOT proof of registration.
    return NativeControlDecision::NativePending;
}
NativeControlDecision ExperimentalNativeRegistrationController::stop() {
    if (!host_) return NativeControlDecision::NotConfigured;
    const bool done = host_->stop();
    if (!done) return NativeControlDecision::StopFailed;
    started_ = false;
    pending_ = false;
    generation_ = 0;
    terminal_observed_ = false;
    callback_unverified_seen_ = false;
    cancel_requested_ = false;
    return NativeControlDecision::Allowed;
}
void ExperimentalNativeRegistrationController::reset_retry_budget_with_explicit_consent() {
    if (!started_ && !pending_) retry_.reset_with_explicit_user_action();
}
NativeControlView ExperimentalNativeRegistrationController::snapshot() const {
    NativeControlView result;
    if (!host_) return result;
    const auto native = host_->snapshot();
    result.control_thread_running = started_ && native.running;
    result.account_provisioned = native.account_provisioned;
    result.register_request_pending = pending_;
    result.cancellation_requested = cancel_requested_;
    result.terminal_failure_seen = terminal_observed_;
    result.callback_unverified_seen = callback_unverified_seen_;
    result.restart_required = terminal_observed_ || cancel_requested_ ||
        (pending_ && !native.running) ||
        native.last_dispatch == RegistrationDispatchKind::Cancelled;
    result.generation = result.control_thread_running ? generation_ : 0;
    result.next_attempt_ms = retry_.next_allowed_ms();
    result.failures = retry_.failures();
    result.last_dispatch = native.last_dispatch;
    // Do not map RegistrationDispatchKind::Confirmed to authenticated SIP:
    // callback proof, credential challenge and server trust are separate.
    result.sip_authenticated = false;
    result.media_verified = false;
    return result;
}
} // namespace axv
