#pragma once
#ifndef _WIN32
#error WindowsProcess is supported on Windows only
#endif
#include <windows.h>
#include <filesystem>

namespace axv {
enum class ProcessStatus { Stopped, Running, Exited, Failed };
class WindowsProcess {
public:
    WindowsProcess() = default;
    WindowsProcess(const WindowsProcess&) = delete;
    WindowsProcess& operator=(const WindowsProcess&) = delete;
    ~WindowsProcess();

    // Starts one explicit executable path. No shell and no user-provided arguments.
    bool start(const std::filesystem::path& executable);
    ProcessStatus poll();
    // Forced termination. A later Baresip control channel must provide graceful exit.
    bool stop();
    DWORD last_error() const { return last_error_; }
    DWORD process_id() const { return process_id_; }

private:
    void release_handles();
    HANDLE job_ = nullptr;
    HANDLE process_ = nullptr;
    DWORD process_id_ = 0;
    DWORD last_error_ = ERROR_SUCCESS;
    ProcessStatus state_ = ProcessStatus::Stopped;
};
}
