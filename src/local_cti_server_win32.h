#pragma once
#include "local_cti_events.h"
#include "local_cti_requests.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace axv {

// Disabled by default; starts only with a valid manually provisioned token.
// Calls are never executed by the network thread; the Win32 GUI must re-check
// the SIP session and original destination when it drains the one-slot queue.
class LocalCtiServer {
public:
    static constexpr unsigned short port = 27652;
    LocalCtiServer() = default;
    LocalCtiServer(const LocalCtiServer&) = delete;
    LocalCtiServer& operator=(const LocalCtiServer&) = delete;
    ~LocalCtiServer();
    bool start(std::string token);
    void stop();
    // Worker posts one nonblocking notification to the registered GUI window.
    // The GUI remains the sole authority to authorize and dispatch SIP calls.
    void set_notification_target(std::uintptr_t window,unsigned int message) {
        notification_message_.store(message,std::memory_order_release);
        notification_window_.store(window,std::memory_order_release);
    }
    bool active() const { return running_.load(); }
    void set_ready(bool ready) {
        ready_.store(ready,std::memory_order_release);
        if(!ready) requests_.reject_pending();
    }
    void observe(const SipRuntimeObservation& state) { journal_.observe(state); }
    std::optional<CtiPendingRequest> take();
    bool complete(std::uint64_t id,bool submitted) { return requests_.complete(id,submitted); }
private:
    void run();
    void handle_client(std::uintptr_t client);
    std::string token_;
    std::uintptr_t listener_ = ~static_cast<std::uintptr_t>(0);
    std::thread worker_;
    std::atomic<bool> stopping_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> ready_{false};
    CtiEventJournal journal_;
    CtiRequestQueue requests_;
    std::atomic<std::uintptr_t> notification_window_{0};
    std::atomic<unsigned int> notification_message_{0};
};
} // namespace axv
