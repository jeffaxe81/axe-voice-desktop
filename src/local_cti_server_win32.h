#pragma once
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
    bool active() const { return running_.load(); }
    void set_ready(bool ready) { ready_.store(ready); }
    std::optional<std::wstring> take();
private:
    void run();
    void handle_client(std::uintptr_t client);
    std::string token_;
    std::uintptr_t listener_ = ~static_cast<std::uintptr_t>(0);
    std::thread worker_;
    std::atomic<bool> stopping_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> ready_{false};
    std::mutex queue_mutex_;
    std::optional<std::wstring> queued_;
};
} // namespace axv
