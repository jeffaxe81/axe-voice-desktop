#pragma once
#include "incoming_ringtone_gate.h"
#include <cstdint>
#include <string>

namespace axv {

// Contract for a FUTURE audited, cancellable incoming-call ringtone renderer.
// start() can fail after a partial activation; callers MUST issue stop() even
// after a failed start. stop() returns true ONLY when output is fully silent.
// Do not route a manual "TESTAR TOQUE" worker through this interface.
class IncomingRingtoneSink {
public:
    virtual ~IncomingRingtoneSink() = default;
    virtual bool start(const std::wstring& endpoint_id) = 0;
    virtual bool stop() = 0;
    virtual bool operational() const { return true; }
};

// The executable intentionally has NO automatic ringtone backend.
// A real sink must be bound only after native Baresip proof + media acceptance.
class DisabledIncomingRingtoneSink final : public IncomingRingtoneSink {
public:
    bool start(const std::wstring&) override { return false; }
    bool stop() override { return true; }
};

enum class IncomingRingtoneOutcome {
    Idle, Started, Stopped, StopPending, StartRejected
};

// All methods must run on the Win32 owning thread. No endpoint IDs are logged,
// persisted, or sent to the native callback queue. On an unacknowledged stop,
// no new device can start; failed starts are not retried for the same call.
class IncomingRingtoneCoordinator final {
public:
    IncomingRingtoneOutcome update(const SipRuntimeObservation& state,
        std::uint64_t current_session, const std::wstring& output_id,
        bool available, IncomingRingtoneSink& sink) {

        if (stop_pending_) {
            gate_.revoke();
            if (!sink.stop()) return IncomingRingtoneOutcome::StopPending;
            stop_pending_ = false;
            playing_ = false;
            return IncomingRingtoneOutcome::Stopped;
        }
        if (playing_ && !sink.operational()) {
            denied_session_ = current_session;
            denied_call_id_ = state.call_id;
            gate_.revoke();
            if (!sink.stop()) {
                stop_pending_ = true;
                return IncomingRingtoneOutcome::StopPending;
            }
            playing_ = false;
            return IncomingRingtoneOutcome::Stopped;
        }
        if (denied_session_ != 0 &&
            denied_session_ == current_session &&
            denied_call_id_ == state.call_id) {
            // Failed starts fail CLOSED until a different call/epoch.
            gate_.revoke();
            return IncomingRingtoneOutcome::Idle;
        }
        const IncomingRingAction intent =
            gate_.observe(state, current_session, output_id, available);
        if (intent == IncomingRingAction::Stop) {
            if (!playing_) return IncomingRingtoneOutcome::Stopped;
            if (!sink.stop()) {
                stop_pending_ = true;
                return IncomingRingtoneOutcome::StopPending;
            }
            playing_ = false;
            return IncomingRingtoneOutcome::Stopped;
        }
        if (intent != IncomingRingAction::Start)
            return IncomingRingtoneOutcome::Idle;
        // A verified call and valid output are both required by the gate.
        if (sink.start(output_id)) {
            playing_ = true;
            return IncomingRingtoneOutcome::Started;
        }
        // A failed start may have produced audio: always acknowledge stop
        // before permitting anything else. Never retry same call automatically.
        denied_session_ = current_session;
        denied_call_id_ = state.call_id;
        gate_.revoke();
        if (!sink.stop()) {
            stop_pending_ = true;
            playing_ = true; // Output status remains unknown until stop ACK.
            return IncomingRingtoneOutcome::StopPending;
        }
        playing_ = false;
        return IncomingRingtoneOutcome::StartRejected;
    }

    IncomingRingtoneOutcome revoke(IncomingRingtoneSink& sink) {
        gate_.revoke();
        if (!playing_ && !stop_pending_)
            return IncomingRingtoneOutcome::Idle;
        if (!sink.stop()) {
            stop_pending_ = true;
            return IncomingRingtoneOutcome::StopPending;
        }
        stop_pending_ = playing_ = false;
        return IncomingRingtoneOutcome::Stopped;
    }
    bool playing() const { return playing_ && !stop_pending_; }
    bool stop_pending() const { return stop_pending_; }

private:
    IncomingRingtoneGate gate_;
    bool playing_ = false;
    bool stop_pending_ = false;
    std::uint64_t denied_session_ = 0;
    std::uint64_t denied_call_id_ = 0;
};
} // namespace axv
