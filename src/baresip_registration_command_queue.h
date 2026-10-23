#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>

namespace axv {

// Cross-thread control requests contain no raw native pointers, SIP URIs,
// passwords or evidence bits. A native loop owner resolves its own UA.
enum class RegistrationCommandKind { SubmitSelectedAccount, Cancel };
struct RegistrationCommand {
    RegistrationCommandKind kind = RegistrationCommandKind::Cancel;
    std::uint64_t generation = 0;
    std::uint32_t timeout_ms = 0;
};

class RegistrationCommandQueue {
public:
    static constexpr std::size_t kCapacity = 16;
    static constexpr std::uint32_t kMinTimeoutMs = 1000;
    static constexpr std::uint32_t kMaxTimeoutMs = 120000;
    struct Batch {
        std::array<RegistrationCommand,kCapacity> commands{};
        std::size_t size = 0;
        std::uint64_t generation = 0;
        bool overflow = false;
    };

    // Owner-thread only. Generation rolls forward, never restarts at one.
    // A prior cancel/overflow must have been drained and handled first.
    std::uint64_t begin_generation() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (open_ || size_ || overflow_ ||
            last_generation_ == std::numeric_limits<std::uint64_t>::max())
            return 0;
        generation_ = ++last_generation_;
        open_ = true;
        return generation_;
    }

    bool submit(std::uint64_t generation, std::uint32_t timeout_ms) {
        if (timeout_ms < kMinTimeoutMs || timeout_ms > kMaxTimeoutMs)
            return false;
        return enqueue({RegistrationCommandKind::SubmitSelectedAccount,
                        generation, timeout_ms});
    }

    bool cancel(std::uint64_t generation) {
        return enqueue({RegistrationCommandKind::Cancel,generation,0});
    }

    // Owner-thread only. A stale/closed generation is never executable.
    Batch drain(std::uint64_t expected_generation) {
        std::lock_guard<std::mutex> lock(mutex_);
        Batch batch;
        if (!expected_generation || generation_ != expected_generation)
            return batch;
        if (!open_ && !size_ && !overflow_) return batch;
        batch.generation = generation_;
        batch.overflow = overflow_;
        if (!overflow_) {
            batch.size = size_;
            for (std::size_t i=0;i<size_;++i) batch.commands[i]=entries_[i];
        }
        size_=0;
        overflow_=false;
        return batch;
    }

    // Owner-thread only. A stop/timeout revokes all future requests on
    // this generation. A new generation requires an explicit begin().
    void invalidate() {
        std::lock_guard<std::mutex> lock(mutex_);
        open_=false;
        size_=0;
        overflow_=false;
    }

private:
    bool enqueue(RegistrationCommand command) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!open_ || !command.generation ||
            command.generation != generation_ || overflow_) return false;
        if (command.kind == RegistrationCommandKind::Cancel) {
            // Cancellation takes priority even when queue is full. No older
            // Submit may run after this point, and no new one is admitted.
            entries_[0]=command;
            size_=1;
            open_=false;
            return true;
        }
        if (size_ == kCapacity) {
            // Fail-closed: do not deliver a partial history to the SDK.
            size_=0;
            open_=false;
            overflow_=true;
            return false;
        }
        entries_[size_++]=command;
        return true;
    }

    std::mutex mutex_;
    std::array<RegistrationCommand,kCapacity> entries_{};
    std::size_t size_=0;
    std::uint64_t generation_=0;
    std::uint64_t last_generation_=0;
    bool open_=false;
    bool overflow_=false;
};
} // namespace axv
