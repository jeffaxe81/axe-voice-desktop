#pragma once

#include <cstdint>
#include <limits>

namespace axv {

// Retry pacing after worker creation fails; does not track running probes.
class AudioInventoryRetry {
public:
    bool ready(std::uint64_t now_ms) const {
        return now_ms >= deadline_ms_;
    }

    void failed(std::uint64_t now_ms) {
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        const auto deadline = now_ms > maximum - 1000 ? maximum : now_ms + 1000;
        if (deadline > deadline_ms_) deadline_ms_ = deadline;
    }

    void started() { reset(); }
    void reset() { deadline_ms_ = 0; }

private:
    std::uint64_t deadline_ms_ = 0;
};

} // namespace axv
