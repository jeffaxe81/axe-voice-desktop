#pragma once
// LAB ONLY: not a real libre/Baresip event-loop implementation.
// Neither account passwords nor UA pointers cross the UI command queue.
#include "baresip_registration_command_dispatcher.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace axv {
struct RegistrationLabSnapshot {
    std::uint64_t generation=0;
    std::uint64_t ticks=0;
    RegistrationDispatchKind last_event=RegistrationDispatchKind::Idle;
    bool running=false;
};

// Caller serializes start/stop and keeps its resolver/pump/UA alive through
// stop(). Never destroy this worker or call stop() from its native pump.
class BaresipRegistrationLabWorker {
public:
    using Resolver=std::function<struct ua*()>;
    using Pump=std::function<void()>;
    using Clock=std::function<std::uint64_t()>;
    BaresipRegistrationLabWorker(Resolver resolver, Pump pump={}, Clock clock={},
                               std::chrono::milliseconds period=std::chrono::milliseconds(10))
      : resolver_(std::move(resolver)),pump_(std::move(pump)),
        clock_(std::move(clock)),period_(period.count()>0?period:std::chrono::milliseconds(10)) {}
    BaresipRegistrationLabWorker(const BaresipRegistrationLabWorker&)=delete;
    BaresipRegistrationLabWorker& operator=(const BaresipRegistrationLabWorker&)=delete;
    ~BaresipRegistrationLabWorker() { (void)stop(); }

    bool start() {
        if(thread_.joinable()) return false;
        stopping_=false;
        { std::lock_guard<std::mutex> lock(mutex_); ready_=false; status_={}; }
        thread_=std::thread([this]{run();});
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock,[this]{return ready_;});
        return status_.running && status_.generation!=0;
    }
    bool submit(std::uint32_t timeout_ms) {
        if(stopping_) return false;
        const auto id=generation();
        if(!id || !queue_.submit(id,timeout_ms)) return false;
        cv_.notify_all();return true;
    }
    bool cancel() {
        if(stopping_) return false;
        const auto id=generation();
        if(!id || !queue_.cancel(id)) return false;
        cv_.notify_all();return true;
    }
    bool stop() {
        if(!thread_.joinable()) return true;
        if(std::this_thread::get_id()==thread_.get_id()) return false;
        stopping_=true;
        const auto id=generation();
        if(id) (void)queue_.cancel(id);
        cv_.notify_all();
        thread_.join();
        return true;
    }
    RegistrationLabSnapshot snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return status_;
    }
    bool await_event(RegistrationDispatchKind expected,
                     std::chrono::milliseconds duration) const {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock,duration,[this,expected]{
            return status_.last_event==expected;
        });
    }
private:
    std::uint64_t generation() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return status_.running?status_.generation:0;
    }
    std::uint64_t now() const {
        if(clock_) return clock_();
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    void run() {
        // SDK-facing state exists only on the dedicated owner thread.
        BaresipEventMailbox mailbox;
        BaresipCallbackLifecycle lifecycle(mailbox);
        BaresipSdkEventAdapter adapter(lifecycle);
        BaresipSdkRegistrationGateway gateway(adapter);
        BaresipRegistrationCommandDispatcher dispatcher(queue_,gateway);
        const auto id=queue_.begin_generation();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            status_.generation=id;
            status_.running=id!=0;
            ready_=true;
        }
        cv_.notify_all();
        while(id && !stopping_) {
            if(pump_) pump_(); // Fake SDK test pump; NOT re_main().
            if(stopping_) break;
            const auto result=dispatcher.tick(id,now(),resolver_?resolver_():nullptr);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++status_.ticks;
                if(result.kind!=RegistrationDispatchKind::Idle &&
                   result.kind!=RegistrationDispatchKind::Pending &&
                   result.kind!=RegistrationDispatchKind::StaleGeneration)
                   status_.last_event=result.kind;
            }
            cv_.notify_all();
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock,period_);
        }
        (void)gateway.stop(); // Before adapter and UA lifetime end.
        queue_.invalidate();
        { std::lock_guard<std::mutex> lock(mutex_); status_.running=false; }
        cv_.notify_all();
    }
    RegistrationCommandQueue queue_;
    Resolver resolver_;
    Pump pump_;
    Clock clock_;
    std::chrono::milliseconds period_;
    std::atomic<bool> stopping_{true};
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    RegistrationLabSnapshot status_{};
    bool ready_=false;
    std::thread thread_;
};
} // namespace axv
