// Windows standalone allocation-failure regression:
// clang++ -std=c++17 tests/test_security_process_resources.cpp src/windows_process.cpp
// Run with the inert child_process executable as the sole argument.
#include "../src/windows_process.h"
#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
bool injecting = false;
bool injected = false;
std::size_t remaining = 0;
void* allocate(std::size_t size) {
    if (injecting && --remaining == 0) {
        injecting = false;
        injected = true;
        throw std::bad_alloc();
    }
    if (void* result = std::malloc(size ? size : 1)) return result;
    throw std::bad_alloc();
}
}
void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 1;
    const std::filesystem::path executable(argv[1]);
    // Warm up filesystem/CRT lazy initialization before measuring ownership.
    {
        axv::WindowsProcess warmup;
        if (!warmup.start(executable) || !warmup.stop()) return 2;
    }
    std::size_t failures = 0;
    for (std::size_t index = 1; index != 512; ++index) {
        DWORD before = 0, after = 0;
        if (!GetProcessHandleCount(GetCurrentProcess(), &before)) return 3;
        bool started = false;
        bool unexpected_exception = false;
        DWORD launch_error = ERROR_SUCCESS;
        axv::ProcessStatus launch_state = axv::ProcessStatus::Stopped;
        injected = false;
        {
            axv::WindowsProcess process;
            remaining = index;
            injecting = true;
            try { started = process.start(executable); }
            catch (...) { unexpected_exception = true; }
            injecting = false;
            launch_error = process.last_error();
            launch_state = process.poll();
            if (injected && !unexpected_exception) {
                if (started || launch_error != ERROR_NOT_ENOUGH_MEMORY ||
                    launch_state != axv::ProcessStatus::Failed) return 9;
                // Reuse the same object after every rejected launch.
                if (!process.start(executable) || !process.stop()) return 10;
            }
            // Destructor must release all resources, including partial starts.
        }
        if (!GetProcessHandleCount(GetCurrentProcess(), &after)) return 4;
        if (unexpected_exception) return 5;
        if (after > before) {
            std::fprintf(stderr,
                "allocation %zu: leaked %lu Win32 handles after destructor\n",
                index, static_cast<unsigned long>(after - before));
            return 6;
        }
        if (injected) { ++failures; continue; }
        if (!started || !failures) return 7;
        std::printf("checked %zu allocation failures; no leaked process handles\n", failures);
        return 0;
    }
    return 8;
}
