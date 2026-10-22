#include "../src/windows_process.h"
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

int wmain(int argc, wchar_t** argv) {
    using namespace axv;
    if (argc != 4) return 1;
    WindowsProcess process;
    if (process.start(L"__axe_voice_nonexistent_process__.exe")) return 2;
    if (process.last_error() == ERROR_SUCCESS) return 3;
    if (!process.start(std::filesystem::path(argv[1]))) {
        std::wcerr << L"CreateProcess failed: " << process.last_error() << L"\n";
        return 4;
    }
    if (process.poll() != ProcessStatus::Running) return 5;
    if (process.process_id() == 0) return 6;
    if (process.start(std::filesystem::path(argv[1]))) return 7;
    if (!process.stop()) return 8;
    if (process.poll() != ProcessStatus::Stopped) return 9;
    if (!process.start(std::filesystem::path(argv[1]))) return 10;
    if (!process.stop()) return 11;

    // The profile name intentionally contains spaces to validate quoted -f arguments.
    const auto profile = std::filesystem::temp_directory_path() /
        (L"AXE Voice lab CI " + std::to_wstring(GetCurrentProcessId()));
    std::error_code ec;
    std::filesystem::create_directories(profile, ec);
    if (ec) return 12;
    if (process.start(std::filesystem::path(argv[1]), profile / L"missing")) return 13;
    if (!process.start(std::filesystem::path(argv[1]), profile)) return 14;
    const auto marker = profile / L"axe-voice-test-child-started";
    bool seen = false;
    for (int i = 0; i != 50; ++i) {
        if (std::filesystem::exists(marker)) { seen = true; break; }
        if (process.poll() != ProcessStatus::Running) break;
        Sleep(100);
    }
    if (!seen) { process.stop(); return 15; }
    if (!process.stop()) return 16;
    std::filesystem::remove_all(profile, ec);
    if (ec) return 17;

    if (!process.start(std::filesystem::path(argv[2]))) return 18;
    bool exited = false;
    for (int i = 0; i != 100; ++i) {
        if (process.poll() == ProcessStatus::Exited) { exited = true; break; }
        Sleep(20);
    }
    if (!exited || !process.exit_code() || *process.exit_code() != 7) return 19;
    if (!process.stop() || process.exit_code().has_value()) return 20;
    if (!process.start(std::filesystem::path(argv[3]))) return 21;
    if (process.stop_gracefully(2000) != GracefulStopResult::ExitedAfterQuit) return 22;
    if (process.poll() != ProcessStatus::Stopped ||
        !process.exit_code() || *process.exit_code() != 0) return 23;
    if (!process.start(std::filesystem::path(argv[1]))) return 24;
    if (process.stop_gracefully(120) != GracefulStopResult::Forced) return 25;
    if (process.poll() != ProcessStatus::Stopped || process.exit_code().has_value()) return 26;
    // Non-blocking shutdown primitive: request returns before the inert child exits.
    if (!process.start(std::filesystem::path(argv[3]))) return 27;
    if (!process.request_quit()) return 28;
    if (process.request_quit()) return 29; // no second command may be written
    bool quit_exited = false;
    for (int i = 0; i != 100; ++i) {
        if (process.poll() == ProcessStatus::Exited) { quit_exited = true; break; }
        Sleep(20);
    }
    if (!quit_exited || !process.exit_code() || *process.exit_code() != 0) return 30;
    if (!process.stop()) return 31;
    if (!process.start(std::filesystem::path(argv[1]))) return 32;
    if (!process.request_quit()) return 33;
    if (!process.stop()) return 34; // forced fallback is still possible
    return 0;
}
