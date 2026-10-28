#include "../src/recent_destinations.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <utility>

namespace {
bool injecting = false;
std::size_t remaining = 0;
bool allocation_failed = false;

void* allocate(std::size_t size) {
    if (injecting) {
        if (remaining == 0) {
            allocation_failed = true;
            throw std::bad_alloc();
        }
        --remaining;
    }
    if (void* memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}

void check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

struct Snapshot {
    std::uint64_t token = 0;
    std::wstring destination;
};
using Snapshots = std::array<Snapshot, axv::RecentDestinations::capacity>;

void exercise_failure_points(bool duplicate) {
    using namespace axv;
    const SipRuntimeObservation observation{RuntimeProcess::Running,
        SipStatus::Registered, SipEvidence::VerifiedNativeApi, 41, 0, 0};
    // Construct every input and expected string before enabling injection.
    std::array<std::wstring, RecentDestinations::capacity> inputs;
    for (std::size_t i = 0; i < inputs.size(); ++i)
        inputs[i] = L"sip:allocation-test-user-" + std::to_wstring(i) + L"@example.com";
    const std::wstring candidate = duplicate ? inputs[2]
        : L"sip:new-allocation-test-user@example.com";
    bool observed_failure = false;
    bool observed_success = false;
    for (std::size_t fail_at = 0; fail_at < 128; ++fail_at) {
        RecentDestinations recent;
        recent.synchronize(observation);
        for (const auto& input : inputs)
            check(recent.remember(observation, input, true), "seed insertion failed");
        Snapshots before;
        check(recent.size() == before.size(), "seed list is not full");
        for (std::size_t i = 0; i < before.size(); ++i) {
            const auto* entry = recent.at(i);
            check(entry != nullptr, "seed entry is missing");
            before[i] = {entry->token, entry->destination};
        }

        remaining = fail_at;
        allocation_failed = false;
        injecting = true;
        const bool accepted = recent.remember(observation, candidate, true);
        injecting = false;

        if (allocation_failed) {
            observed_failure = true;
            check(!accepted, "remember accepted a failed allocation");
            check(recent.size() == before.size(), "allocation failure changed list size");
            for (std::size_t i = 0; i < before.size(); ++i) {
                const auto* entry = recent.at(i);
                check(entry && entry->token == before[i].token &&
                    entry->destination == before[i].destination,
                    "allocation failure changed an existing entry or MRU order");
                const auto* found = recent.find(before[i].token, observation);
                check(found && found->destination == before[i].destination,
                    "allocation failure invalidated an existing token");
            }
        } else {
            check(accepted, "remember rejected insertion without allocation failure");
            check(recent.size() == before.size(), "successful insertion changed full capacity");
            const auto* first = recent.at(0);
            check(first && first->destination == candidate, "successful insertion is not MRU");
            for (const auto& entry : before)
                check(first->token != entry.token, "successful insertion reused an old token");
            observed_success = true;
            break;
        }
    }
    check(observed_failure, "fault injection did not exercise any allocation failure");
    check(observed_success, "fault sweep never reached successful insertion");
}
} // namespace

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    using axv::RecentDestinations;
    using axv::SipRuntimeObservation;
    static_assert(noexcept(std::declval<RecentDestinations&>().remember(
        std::declval<const SipRuntimeObservation&>(),
        std::declval<const std::wstring&>(), true)), "remember must be noexcept");
    exercise_failure_points(false);
    exercise_failure_points(true);
}
