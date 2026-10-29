// Exercises real Windows named-object lifetimes, including simultaneous admission.
#include "../src/native_install_guard.h"
#include <windows.h>
#include <sddl.h>
#include <atomic>
#include <cstdio>
#include <thread>
#include <type_traits>

namespace {
using Result = axv::NativeInstallGuardResult;
struct Handle {
    HANDLE value = nullptr;
    ~Handle() { if (value) CloseHandle(value); }
};
struct SetupReservation {
    HANDLE value = nullptr;
    ~SetupReservation() { release(); }
    void release() { if (value) CloseHandle(value); value = nullptr; }
    Result acquire(const axv::NativeInstallGuardNames& names) {
        release();
        HANDLE candidate = CreateMutexW(nullptr, FALSE, names.setup.c_str());
        const DWORD creation_error = GetLastError();
        if (!candidate) return Result::Failed;
        if (creation_error == ERROR_ALREADY_EXISTS) {
            CloseHandle(candidate);
            return Result::Busy;
        }
        HANDLE app = OpenMutexW(SYNCHRONIZE, FALSE, names.application.c_str());
        const DWORD probe_error = GetLastError();
        if (app) CloseHandle(app);
        if (app || probe_error != ERROR_FILE_NOT_FOUND) {
            CloseHandle(candidate);
            return app ? Result::Busy : Result::Failed;
        }
        value = candidate;
        return Result::Acquired;
    }
};
axv::NativeInstallGuardNames scoped_names(const wchar_t* scope) {
    const std::wstring prefix = L"Local\\AXEVoiceNativeCandidate.GuardTest." +
        std::to_wstring(GetCurrentProcessId()) + L"." + scope;
    return {prefix + L".Running", prefix + L".Setup"};
}
bool absent(const std::wstring& name) {
    Handle object{OpenMutexW(SYNCHRONIZE, FALSE, name.c_str())};
    return !object.value && GetLastError() == ERROR_FILE_NOT_FOUND;
}
int fail(int code, const char* description) {
    std::fprintf(stderr, "install guard %d: %s\n", code, description);
    return code;
}
}

int main() {
    static_assert(!std::is_copy_constructible<axv::NativeInstallGuard>::value);
    const auto defaults = axv::native_install_guard_names();
    if (!defaults || defaults->application.find(L"Global\\AXEVoiceNativeCandidate.Running.S-1-") != 0 ||
        defaults->setup.find(L"Global\\AXEVoiceNativeCandidate.Setup.S-1-") != 0)
        return fail(1, "production names must use global namespace and token SID");
    if (defaults->application.substr(defaults->application.find(L"S-1-")) !=
        defaults->setup.substr(defaults->setup.find(L"S-1-")))
        return fail(2, "both markers must use the same SID");
    const auto names = scoped_names(L"lifetime");
    axv::NativeInstallGuard first, second;
    SetupReservation setup;
    if (first.acquire(names) != Result::Acquired || second.acquire(names) != Result::Acquired)
        return fail(3, "multiple application holders must be admitted");
    if (setup.acquire(names) != Result::Busy || !absent(names.setup))
        return fail(4, "setup must reject active application and release reservation");
    first.release();
    if (setup.acquire(names) != Result::Busy)
        return fail(5, "remaining application holder must still block setup");
    second.release();
    if (!absent(names.application) || setup.acquire(names) != Result::Acquired)
        return fail(6, "last release must permit setup retry");
    if (first.acquire(names) != Result::Busy || !absent(names.application))
        return fail(7, "setup must reject app and rejected app must release marker");
    SetupReservation competing_setup;
    if (competing_setup.acquire(names) != Result::Busy)
        return fail(8, "another setup must not share reservation");
    const auto other_names = scoped_names(L"other-scope");
    if (second.acquire(other_names) != Result::Acquired)
        return fail(9, "distinct scope must remain independent");
    second.release();
    setup.release();
    if (first.acquire(names) != Result::Acquired)
        return fail(10, "same app guard must retry after setup release");
    first.release();
    first.release();
    {
        axv::NativeInstallGuard automatic;
        if (automatic.acquire(names) != Result::Acquired) return fail(11, "destructor setup");
    }
    if (!absent(names.application)) return fail(12, "destructor must close marker");
    // A named event is incompatible with a mutex: creation/probing must fail closed.
    {
        Handle event{CreateEventW(nullptr, TRUE, FALSE, names.application.c_str())};
        if (!event.value || first.acquire(names) != Result::Failed)
            return fail(13, "incompatible application object must fail closed");
    }
    {
        Handle event{CreateEventW(nullptr, TRUE, FALSE, names.setup.c_str())};
        if (!event.value || first.acquire(names) != Result::Failed || !absent(names.application))
            return fail(14, "incompatible setup object must fail closed without leaked app marker");
    }
    // Retain the original full-access handle while denying subsequent opens.
    // Closing it removes this isolated object, so no restrictive object remains.
    {
        Handle denied_setup{CreateMutexW(nullptr, FALSE, names.setup.c_str())};
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!denied_setup.value ||
            !ConvertStringSecurityDescriptorToSecurityDescriptorW(
                L"D:P(D;;GA;;;WD)", SDDL_REVISION_1, &descriptor, nullptr))
            return fail(23, "access-denied test security descriptor creation");
        const BOOL secured = SetKernelObjectSecurity(
            denied_setup.value, DACL_SECURITY_INFORMATION, descriptor);
        LocalFree(descriptor);
        if (!secured) return fail(24, "access-denied test DACL application unsupported or failed");
        Handle probe{OpenMutexW(SYNCHRONIZE, FALSE, names.setup.c_str())};
        const DWORD probe_error = GetLastError();
        if (probe.value || probe_error != ERROR_ACCESS_DENIED)
            return fail(27, "restricted setup object must produce actual access denied");
        if (first.acquire(names) != Result::Failed || !absent(names.application))
            return fail(25, "access-denied setup probe must fail closed without app marker");
    }
    if (!absent(names.setup)) return fail(26, "restricted setup object must disappear on close");
    // Event barriers release both real participants together; a second barrier keeps
    // admitted markers alive until both outcomes have been observed.
    for (unsigned iteration = 0; iteration != 200; ++iteration) {
        Handle start{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        Handle finish{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        if (!start.value || !finish.value) return fail(15, "race event creation");
        std::atomic<unsigned> ready{0}, completed{0};
        Result app_result = Result::Failed, setup_result = Result::Failed;
        std::thread app([&] {
            axv::NativeInstallGuard guard;
            ++ready;
            WaitForSingleObject(start.value, INFINITE);
            app_result = guard.acquire(names);
            ++completed;
            WaitForSingleObject(finish.value, INFINITE);
        });
        std::thread installer([&] {
            SetupReservation reservation;
            ++ready;
            WaitForSingleObject(start.value, INFINITE);
            setup_result = reservation.acquire(names);
            ++completed;
            WaitForSingleObject(finish.value, INFINITE);
        });
        while (ready.load() != 2) std::this_thread::yield();
        SetEvent(start.value);
        while (completed.load() != 2) std::this_thread::yield();
        const bool both_admitted = app_result == Result::Acquired && setup_result == Result::Acquired;
        SetEvent(finish.value);
        app.join();
        installer.join();
        if (both_admitted) return fail(16, "racing setup and application both admitted");
        if (app_result == Result::Failed || setup_result == Result::Failed)
            return fail(17, "race encountered unexpected OS failure");
        if (!absent(names.application) || !absent(names.setup))
            return fail(18, "race participants leaked markers");
    }
    DWORD before = 0, after = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &before)) return fail(19, "initial handle count");
    for (unsigned iteration = 0; iteration != 500; ++iteration) {
        axv::NativeInstallGuard guard;
        SetupReservation reservation;
        if (reservation.acquire(names) != Result::Acquired || guard.acquire(names) != Result::Busy)
            return fail(20, "resource loop refusal");
        reservation.release();
        if (guard.acquire(names) != Result::Acquired) return fail(21, "resource loop retry");
    }
    if (!GetProcessHandleCount(GetCurrentProcess(), &after) || after != before)
        return fail(22, "repeated refusal and retry leaked handles");
    std::puts("real Win32 marker admission, races, retry, and handle lifetime checks passed");
    return 0;
}
