#pragma once
#include <thread>

namespace axv {

// The Baresip C API must be controlled from the thread that owns its event
// loop. Capture thread identity at construction, never silently rebind.
class BaresipNativeThreadGuard {
public:
    BaresipNativeThreadGuard() : owner_(std::this_thread::get_id()) {}
    BaresipNativeThreadGuard(const BaresipNativeThreadGuard&) = delete;
    BaresipNativeThreadGuard& operator=(const BaresipNativeThreadGuard&) = delete;

    bool is_owner() const noexcept {
        return std::this_thread::get_id() == owner_;
    }
private:
    const std::thread::id owner_;
};
} // namespace axv
