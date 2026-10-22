#pragma once
#ifndef _WIN32
#error WindowsProcess is supported on Windows only
#endif
#include <windows.h>
#include <filesystem>
#include <optional>

namespace axv {
enum class ProcessStatus { Stopped, Running, Exited, Failed };
enum class GracefulStopResult { AlreadyStopped, ExitedAfterQuit, Forced, Failed };
class WindowsProcess {
public:
    WindowsProcess() = default;
    WindowsProcess(const WindowsProcess&) = delete;
    WindowsProcess& operator=(const WindowsProcess&) = delete;
    ~WindowsProcess();

    // Starts a fixed executable (no shell). If supplied, profile is passed as -f <directory>.
    // No SIP password, account information or arbitrary caller arguments cross the command line.
    bool start(const std::filesystem::path& executable,
               const std::filesystem::path& config_directory = {});
    ProcessStatus poll();
    // Forced termination. A later Baresip control channel must provide graceful exit.
    GracefulStopResult stop_gracefully(DWORD timeout_ms = 1500);
    bool stop();
    DWORD last_error() const { return last_error_; }
    DWORD process_id() const { return process_id_; }
    std::optional<DWORD> exit_code() const { return exit_code_; }

private:
    void release_handles();
    HANDLE job_ = nullptr;
    HANDLE process_ = nullptr;
    HANDLE stdin_write_ = nullptr;
    DWORD process_id_ = 0;
    DWORD last_error_ = ERROR_SUCCESS;
    std::optional<DWORD> exit_code_;
    ProcessStatus state_ = ProcessStatus::Stopped;
};
}
