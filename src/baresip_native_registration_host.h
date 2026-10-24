#pragma once
// Experimental control-plane wiring between a libre re_main owner thread
// and the isolated Baresip registration gateway. NOT linked to AXEVoice GUI.
#include "baresip_native_libre_reactor.h"
#include "baresip_registration_command_dispatcher.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <mutex>
#include <utility>

namespace axv {
struct NativeRegistrationHostSnapshot {
    bool running = false;
    std::uint64_t command_generation = 0;
    std::uint64_t ticks = 0;
    RegistrationDispatchKind last_dispatch = RegistrationDispatchKind::Idle;
    LibreReactorFault reactor_fault = LibreReactorFault::None;
};

// The caller provisions and owns the UA and must keep it alive until stop()
// joins. Resolver and optional monotonic clock are invoked ONLY by re_main's
// owner thread. start()/stop()/destructor must be externally serialized and
// MUST NOT be called from a native callback.
class BaresipNativeRegistrationHost {
public:
    using ResolveSelectedUa = std::function<struct ua*()>;
    using MonotonicClock = std::function<std::uint64_t()>;

    explicit BaresipNativeRegistrationHost(
        ResolveSelectedUa resolver,
        MonotonicClock clock = {},
        std::chrono::milliseconds cadence = std::chrono::milliseconds(20))
      : resolver_(std::move(resolver)), clock_(std::move(clock)),
        reactor_([this] { on_tick(); },
                 [this] { on_start(); },
                 [this] { on_stop(); }, cadence) {}

    BaresipNativeRegistrationHost(const BaresipNativeRegistrationHost&)=delete;
    BaresipNativeRegistrationHost& operator=(const BaresipNativeRegistrationHost&)=delete;
    ~BaresipNativeRegistrationHost() { (void)stop(); }

    bool start() {
        if (reactor_.snapshot().running) return false;
        accepting_.store(false);
        const bool launched=reactor_.start();
        if (!launched) {
            (void)reactor_.stop(); // Join even on initialization failure.
            return false;
        }
        return accepting_.load();
    }

    // UI-facing actions transmit only timeout + an opaque generation token.
    // No pointers, SIP passwords or account identifiers cross the queue.
    std::uint64_t generation() const { return active_generation_.load(); }

    bool submit(std::uint32_t timeout_ms) {
        return submit_for_generation(generation(),timeout_ms);
    }

    bool submit_for_generation(std::uint64_t id,std::uint32_t timeout_ms) {
        if (!accepting_.load() || !id || id!=generation() ||
            !queue_.submit(id,timeout_ms)) return false;
        if (reactor_.wake()) return true;
        // Wake failure must never leave an unacknowledged new Submit armed.
        // The reactor timer will observe this cancellation on the owner.
        (void)queue_.cancel(id);
        return false;
    }

    bool cancel() {
        const auto id=generation();
        if (!accepting_.load() || !id || !queue_.cancel(id)) return false;
        (void)reactor_.wake(); // Native timer is a fallback if wake fails.
        return true;          // Accepted for dispatch, not yet completed.
    }

    bool stop() {
        // Stop submissions *before* revoking the native generation.
        accepting_.store(false);
        queue_.invalidate();
        const bool done=reactor_.stop(); // Joins after owner-side UA cleanup.
        if (done) active_generation_.store(0);
        return done;
    }

    NativeRegistrationHostSnapshot snapshot() const {
        NativeRegistrationHostSnapshot out;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            out=state_;
        }
        const auto r=reactor_.snapshot();
        out.running=r.running;
        out.reactor_fault=r.fault;
        return out;
    }

private:
    struct OwnerState {
        BaresipEventMailbox mailbox;
        BaresipCallbackLifecycle lifecycle{mailbox};
        BaresipSdkEventAdapter adapter{lifecycle};
        BaresipSdkRegistrationGateway gateway{adapter};
        BaresipRegistrationCommandDispatcher dispatcher;
        std::uint64_t generation=0;
        explicit OwnerState(RegistrationCommandQueue& queue)
          : dispatcher(queue,gateway) {}
    };

    std::uint64_t now_ms() const {
        if (clock_) return clock_();
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    void on_start() {
        // Runs only on libre's re_main thread, before accepting UI commands.
        owner_=std::make_unique<OwnerState>(queue_);
        const auto id=queue_.begin_generation();
        if (!id) { owner_.reset(); throw std::runtime_error("no generation"); }
        owner_->generation=id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_={};
            state_.command_generation=id;
        }
        active_generation_.store(id);
        accepting_.store(true);
    }

    void on_tick() {
        if (!owner_) throw std::runtime_error("owner missing");
        const auto result=owner_->dispatcher.tick(
            owner_->generation,now_ms(),resolver_?resolver_():nullptr);
        std::lock_guard<std::mutex> lock(mutex_);
        ++state_.ticks;
        if (result.kind!=RegistrationDispatchKind::Idle &&
            result.kind!=RegistrationDispatchKind::Pending &&
            result.kind!=RegistrationDispatchKind::StaleGeneration)
            state_.last_dispatch=result.kind;
    }

    void on_stop() {
        // Clean up callbacks, watchdog and owner pointers on the native
        // thread BEFORE allowing libre_close or any UI restart.
        accepting_.store(false);
        queue_.invalidate();
        if (owner_) {
            (void)owner_->gateway.stop();
            owner_.reset();
        }
        active_generation_.store(0);
    }

    RegistrationCommandQueue queue_;
    ResolveSelectedUa resolver_;
    MonotonicClock clock_;
    BaresipNativeLibreReactor reactor_;
    mutable std::mutex mutex_;
    NativeRegistrationHostSnapshot state_{};
    std::unique_ptr<OwnerState> owner_; // Native-thread-only, never exposed.
    std::atomic<std::uint64_t> active_generation_{0};
    std::atomic<bool> accepting_{false};
};
} // namespace axv
