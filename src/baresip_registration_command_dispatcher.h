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
            if (submitted!=NativeRegistrationSubmission::Submitted ||
                !gateway_.arm_registration_deadline(
                    monotonic_ms,command.timeout_ms)) {
                gateway_.stop();
                queue_.invalidate();
                return {RegistrationDispatchKind::Failed,submitted};
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
