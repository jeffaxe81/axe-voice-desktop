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
    bool account_provisioned = false;
    bool accepting_requests = false; // pointer-free admission status only
};

// The caller provisions and owns the UA and must keep it alive until stop()
// joins. Resolver and optional monotonic clock are invoked ONLY by re_main's
// owner thread. start()/stop()/destructor must be externally serialized and
// MUST NOT be called from a native callback.
class BaresipNativeRegistrationHost {
public:
    using ResolveSelectedUa = std::function<struct ua*()>;
    using MonotonicClock = std::function<std::uint64_t()>;
    // The callback owner supplies credentials ONLY inside create() on the
    // libre event-loop thread, never to this host or its UI command queue.
    // destroy() must be noexcept, must not call stop(), and must release
    // its UA on the SAME thread after gateway.stop()/callback quiescence.
    struct NativeUaLifecycleHooks {
        std::function<struct ua*()> create;
        std::function<void(struct ua*)> destroy;
    };

    explicit BaresipNativeRegistrationHost(
        ResolveSelectedUa resolver,
        MonotonicClock clock = {},
        std::chrono::milliseconds cadence = std::chrono::milliseconds(20))
      : resolver_(std::move(resolver)), clock_(std::move(clock)),
        reactor_([this] { on_tick(); },
                 [this] { on_start(); },
                 [this] { on_stop(); }, cadence) {}

    // Stronger ownership mode for future audited SIP account provisioning.
    // No native pointer or credential passes through submit()/snapshot().
    // A partial pair of hooks fails closed during start().
    explicit BaresipNativeRegistrationHost(
        NativeUaLifecycleHooks hooks,
        MonotonicClock clock = {},
        std::chrono::milliseconds cadence = std::chrono::milliseconds(20))
      : BaresipNativeRegistrationHost(ResolveSelectedUa{},
                                     std::move(clock), cadence) {
        hooks_ = std::move(hooks);
    }

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
        out.accepting_requests = r.running && accepting_.load() &&
            r.fault == LibreReactorFault::None;
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
        // Provision exactly once per native-loop generation. Neither the
        // UI nor the generic legacy resolver gets to see an owned UA.
        if (hooks_.create || hooks_.destroy) {
            if (!hooks_.create || !hooks_.destroy) {
                owner_.reset();
                throw std::runtime_error("incomplete native UA hooks");
            }
            try {
                owner_ua_=hooks_.create();
            } catch (...) {
                owner_.reset();
                throw;
            }
            if (!owner_ua_) {
                owner_.reset();
                throw std::runtime_error("native UA provisioning rejected");
            }
        }
        const auto id=queue_.begin_generation();
        if (!id) {
            // A failed admission must never leave a provisioned UA alive.
            auto* selected=std::exchange(owner_ua_,nullptr);
            if (selected) hooks_.destroy(selected);
            owner_.reset();
            throw std::runtime_error("no generation");
        }
        owner_->generation=id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_={};
            state_.command_generation=id;
            state_.account_provisioned=owner_ua_!=nullptr;
        }
        active_generation_.store(id);
        accepting_.store(true);
    }

    void on_tick() {
        if (!owner_) throw std::runtime_error("owner missing");
        // Only owner mode may reuse its generation's private UA pointer.
        // Legacy resolver mode stays for isolated host tests/migration.
        auto* selected=(hooks_.create || hooks_.destroy)
            ? owner_ua_ : (resolver_?resolver_():nullptr);
        const auto result=owner_->dispatcher.tick(
            owner_->generation,now_ms(),selected);
        // Native terminal decisions revoke cross-thread admission BEFORE
        // publishing the diagnostic result. Stop/join is still required.
        const bool terminal = registration_dispatch_terminal(result.kind);
        if (terminal) accepting_.store(false);
        std::lock_guard<std::mutex> lock(mutex_);
        ++state_.ticks;
        // Preserve the first terminal outcome of a revoked generation,
        // including if a callback or a stale queue tick appears later.
        if (!registration_dispatch_terminal(state_.last_dispatch) &&
            result.kind != RegistrationDispatchKind::Idle &&
            result.kind != RegistrationDispatchKind::Pending)
            state_.last_dispatch = result.kind;
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
        auto* selected=std::exchange(owner_ua_,nullptr);
        active_generation_.store(0);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.account_provisioned=false;
        }
        // The UA cannot be destroyed until gateway.stop() has
        // unregistered its SDK callback and awaited the callback lease.
        if (selected) hooks_.destroy(selected);
    }

    RegistrationCommandQueue queue_;
    ResolveSelectedUa resolver_;
    MonotonicClock clock_;
    NativeUaLifecycleHooks hooks_{};
    struct ua* owner_ua_ = nullptr; // Native thread ONLY; never in snapshot.
    BaresipNativeLibreReactor reactor_;
    mutable std::mutex mutex_;
    NativeRegistrationHostSnapshot state_{};
    std::unique_ptr<OwnerState> owner_; // Native-thread-only, never exposed.
    std::atomic<std::uint64_t> active_generation_{0};
    std::atomic<bool> accepting_{false};
};
} // namespace axv
