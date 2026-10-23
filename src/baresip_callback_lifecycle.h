#pragma once
#include "baresip_callback_ingress.h"
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>

namespace axv {

// Callback admission and quiescence gate. NOT a provenance authenticator.
// The caller must unregister its native Baresip callback before destroying
// the owner of this fence, and call close()/wait() outside any admitted callback.
class BaresipCallbackFence {
public:
    class Lease {
    public:
        Lease() = default;
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Lease(Lease&& other) noexcept : owner_(std::exchange(other.owner_, nullptr)) {}
        Lease& operator=(Lease&& other) noexcept {
            if (this != &other) {
                release();
                owner_ = std::exchange(other.owner_, nullptr);
            }
            return *this;
        }
        ~Lease() { release(); }
        explicit operator bool() const { return owner_ != nullptr; }
        void release() {
            if (auto* owner = std::exchange(owner_, nullptr))
                owner->release_one();
        }
    private:
        explicit Lease(BaresipCallbackFence* owner) : owner_(owner) {}
        BaresipCallbackFence* owner_ = nullptr;
        friend class BaresipCallbackFence;
    };

    std::uint64_t begin() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (accepting_ || closing_ || in_flight_ || generation_ ==
            std::numeric_limits<std::uint64_t>::max()) return 0;
        accepting_ = true;
        return ++generation_;
    }

    Lease admit(std::uint64_t captured_generation) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!accepting_ || closing_ || !captured_generation ||
            captured_generation != generation_) return {};
        ++in_flight_;
        return Lease(this);
    }

    // Immediately reject all subsequent callback admissions; may be invoked
    // while existing callback leases are still alive.
    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        accepting_ = false;
        closing_ = true;
    }

    // Waits for previously admitted callbacks to return, then permits begin().
    // NEVER invoke on an admitted callback thread: that would deadlock.
    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return in_flight_ == 0; });
        closing_ = false;
    }

    void close_and_wait() {
        close();
        wait();
    }

private:
    void release_one() {
        std::lock_guard<std::mutex> lock(mutex_);
        --in_flight_;
        if (in_flight_ == 0) cv_.notify_all();
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::uint64_t generation_ = 0;
    std::uint64_t in_flight_ = 0;
    bool accepting_ = false;
    bool closing_ = false;
};

// Auditable unit-tested boundary for a future in-process adapter. No C API
// callback is registered here. No code here issues VerifiedNativeApi.
class BaresipCallbackLifecycle {
public:
    struct CapturedSession {
        std::uint64_t fence_generation = 0;
        std::uint64_t mailbox_session = 0;
        explicit operator bool() const {
            return fence_generation != 0 && mailbox_session != 0;
        }
    };
    explicit BaresipCallbackLifecycle(BaresipEventMailbox& mailbox)
        : ingress_(mailbox) {}

    CapturedSession begin() {
        std::lock_guard<std::mutex> lock(lifecycle_);
        if (current_) return {};
        const auto generation = fence_.begin();
        if (!generation) return {};
        const auto session = ingress_.begin_session();
        if (!session) {
            fence_.close_and_wait();
            return {};
        }
        current_ = {generation, session};
        return current_;
    }

    // Lease spanning the entire native C callback, including SDK argument
    // reads; shutdown waits for this lease after handler unregistration.
    BaresipCallbackFence::Lease hold_native_callback(CapturedSession captured) {
        return captured ? fence_.admit(captured.fence_generation)
                        : BaresipCallbackFence::Lease{};
    }

    bool registration(CapturedSession captured, BaresipEventKind event) {
        auto lease = fence_.admit(captured.fence_generation);
        return lease && ingress_.registration(captured.mailbox_session, event);
    }
    bool call_started(CapturedSession captured, std::uint64_t native_key,
                      BaresipEventKind event) {
        auto lease = fence_.admit(captured.fence_generation);
        return lease && ingress_.call_started(captured.mailbox_session, native_key, event);
    }
    bool call_changed(CapturedSession captured, std::uint64_t native_key,
                      BaresipEventKind event) {
        auto lease = fence_.admit(captured.fence_generation);
        return lease && ingress_.call_changed(captured.mailbox_session, native_key, event);
    }

    // Fail closed from a native callback without waiting for itself to return.
    // The management owner must still explicitly revoke_and_wait() at shutdown.
    void reject_current() {
        std::lock_guard<std::mutex> lock(lifecycle_);
        fence_.close();
        ingress_.revoke();
    }

    bool is_live(CapturedSession captured) {
        auto lease = fence_.admit(captured.fence_generation);
        return lease && ingress_.session() == captured.mailbox_session;
    }

    // Unregister the native callback FIRST to prevent new native C API entries;
    // then use this operation to reject queued/stale work, drop pending events
    // and wait for callbacks already admitted to finish. Not callable from one
    // of those callbacks. The native adapter must serialize its registrations
    // with begin()/revoke_and_wait() and maintain object lifetime externally.
    void revoke_and_wait() {
        // Do not hold lifecycle_ while waiting for admitted callbacks:
        // an in-flight callback may need reject_current() to finish.
        {
            std::lock_guard<std::mutex> lock(lifecycle_);
            fence_.close();
            ingress_.revoke();
        }
        fence_.wait(); // stop() only, never on a native callback thread.
        {
            std::lock_guard<std::mutex> lock(lifecycle_);
            current_ = {};
        }
    }

private:
    std::mutex lifecycle_;
    BaresipCallbackFence fence_;
    BaresipCallbackIngress ingress_;
    CapturedSession current_{};
};

} // namespace axv
