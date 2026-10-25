#pragma once
#include "baresip_registration_command_queue.h"
#include "baresip_sdk_registration_gateway.h"
#include <cstdint>
#include <thread>

namespace axv {

// Does NOT create a native event loop. The loop owner constructs this on its
// own thread and invokes tick() with a UA it securely owns and a monotonic
// clock reading. GUI threads may only submit pointer-free requests to queue.
enum class RegistrationDispatchKind {
    Idle, Submitted, Cancelled, DeadlineExpired, Confirmed,
    Failed, OverflowStopped, WrongOwnerThread, StaleGeneration, Pending,
    SupersededDuringDispatch, RegistrarRejected
};
inline bool registration_dispatch_terminal(RegistrationDispatchKind kind) noexcept {
    switch (kind) {
    case RegistrationDispatchKind::Cancelled:
    case RegistrationDispatchKind::DeadlineExpired:
    case RegistrationDispatchKind::RegistrarRejected:
    case RegistrationDispatchKind::Failed:
    case RegistrationDispatchKind::OverflowStopped:
    case RegistrationDispatchKind::WrongOwnerThread:
    case RegistrationDispatchKind::StaleGeneration:
    case RegistrationDispatchKind::SupersededDuringDispatch:
        return true;
    default:
        return false;
    }
}
inline bool registration_dispatch_failure(RegistrationDispatchKind kind) noexcept {
    return registration_dispatch_terminal(kind) &&
           kind != RegistrationDispatchKind::Cancelled;
}

struct RegistrationDispatchOutcome {
    RegistrationDispatchKind kind=RegistrationDispatchKind::Idle;
    NativeRegistrationSubmission submission=NativeRegistrationSubmission::BridgeUnavailable;
};

class BaresipRegistrationCommandDispatcher {
public:
    BaresipRegistrationCommandDispatcher(
        RegistrationCommandQueue& queue,
        BaresipSdkRegistrationGateway& gateway)
        : queue_(queue), gateway_(gateway), owner_(std::this_thread::get_id()) {}

    RegistrationDispatchOutcome tick(std::uint64_t generation,
                                     std::uint64_t monotonic_ms,
                                     struct ua* owner_selected_ua) {
        if (std::this_thread::get_id()!=owner_)
            return {RegistrationDispatchKind::WrongOwnerThread,{}};
        const auto batch=queue_.drain(generation);
        if (batch.generation!=generation || !generation)
            return {RegistrationDispatchKind::StaleGeneration,{}};
        if (batch.overflow) {
            gateway_.stop();
            queue_.invalidate();
            return {RegistrationDispatchKind::OverflowStopped,{}};
        }
        RegistrationDispatchOutcome result{};
        for (std::size_t i=0;i<batch.size;++i) {
            const auto& command=batch.commands[i];
            if (command.generation!=generation) {
                gateway_.stop();
                queue_.invalidate();
                return {RegistrationDispatchKind::Failed,{}};
            }
            if (command.kind==RegistrationCommandKind::Cancel) {
                gateway_.stop();
                queue_.invalidate();
                return {RegistrationDispatchKind::Cancelled,{}};
            }
            if (!queue_.still_admitted(batch)) {
                gateway_.stop();
                queue_.invalidate();
                return {RegistrationDispatchKind::SupersededDuringDispatch,{}};
            }
            const auto submitted=gateway_.submit(owner_selected_ua);
            // The SDK may dispatch events synchronously while a UI thread
            // cancels or overflows the queue. Detect that invalidation BEFORE
            // claiming submission success or arming a deadline.
            if (!queue_.still_admitted(batch)) {
                gateway_.stop();
                queue_.invalidate();
                return {RegistrationDispatchKind::SupersededDuringDispatch,submitted};
            }
            // A synchronous REGISTER_FAIL is not a generic submission
            // error. Preserve the native registrar rejection for diagnostics.
            const bool rejected =
                submitted == NativeRegistrationSubmission::RegistrarRejectedSynchronously ||
                gateway_.registrar_rejection_seen();
            if (rejected) {
                gateway_.stop();
                queue_.invalidate();
                return {RegistrationDispatchKind::RegistrarRejected,submitted};
            }
            if (submitted!=NativeRegistrationSubmission::Submitted ||
                !gateway_.arm_registration_deadline(
                    monotonic_ms,command.timeout_ms)) {
                // A failure can arrive between submit and arming a
                // deadline; retain its provenance before tearing down.
                const bool rejected_while_arming = gateway_.registrar_rejection_seen();
                gateway_.stop();
                queue_.invalidate();
                return {rejected_while_arming ?
                            RegistrationDispatchKind::RegistrarRejected :
                            RegistrationDispatchKind::Failed,submitted};
            }
            if (!queue_.still_admitted(batch)) {
                gateway_.stop();
                queue_.invalidate();
                return {RegistrationDispatchKind::SupersededDuringDispatch,submitted};
            }
            result={RegistrationDispatchKind::Submitted,submitted};
        }
        const auto deadline=gateway_.poll_registration_deadline(monotonic_ms);
        switch (deadline) {
        case RegistrationDeadlineVerdict::RegistrarRejected:
            queue_.invalidate();
            return {RegistrationDispatchKind::RegistrarRejected,{}};
        case RegistrationDeadlineVerdict::Expired:
        case RegistrationDeadlineVerdict::ClockRegressed:
        case RegistrationDeadlineVerdict::SessionRevoked:
            queue_.invalidate();
            return {RegistrationDispatchKind::DeadlineExpired,{}};
        case RegistrationDeadlineVerdict::CallbackConfirmed:
            return {RegistrationDispatchKind::Confirmed,{}};
        case RegistrationDeadlineVerdict::Pending:
            return batch.size? result :
                RegistrationDispatchOutcome{RegistrationDispatchKind::Pending,{}};
        default:
            return result;
        }
    }
private:
    RegistrationCommandQueue& queue_;
    BaresipSdkRegistrationGateway& gateway_;
    const std::thread::id owner_;
};
} // namespace axv
