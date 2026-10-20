#include "windows_process.h"
#include <string>
#include <vector>

namespace axv {
WindowsProcess::~WindowsProcess() { stop(); }

void WindowsProcess::release_handles() {
    if (process_) { CloseHandle(process_); process_ = nullptr; }
    if (job_) { CloseHandle(job_); job_ = nullptr; }
    process_id_ = 0;
}

bool WindowsProcess::start(const std::filesystem::path& executable) {
    if (poll() == ProcessStatus::Running) {
        last_error_ = ERROR_ALREADY_EXISTS;
        return false;
    }
    stop();
    last_error_ = ERROR_SUCCESS;

    std::error_code error;
    const auto absolute = std::filesystem::absolute(executable, error);
    if (error || !std::filesystem::is_regular_file(absolute, error) || error) {
        last_error_ = ERROR_FILE_NOT_FOUND;
        state_ = ProcessStatus::Failed;
        return false;
    }

    job_ = CreateJobObjectW(nullptr, nullptr);
    if (!job_) {
        last_error_ = GetLastError();
        state_ = ProcessStatus::Failed;
        return false;
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job_, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        last_error_ = GetLastError();
        release_handles();
        state_ = ProcessStatus::Failed;
        return false;
    }

    // Explicit lpApplicationName prevents searching PATH or invoking cmd.exe.
    std::wstring command = L"\"" + absolute.wstring() + L"\"";
    std::vector<wchar_t> command_buffer(command.begin(), command.end());
    command_buffer.push_back(L'\0');
    const auto working_directory = absolute.parent_path().wstring();
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info = {};
    if (!CreateProcessW(absolute.c_str(), command_buffer.data(), nullptr, nullptr,
                        FALSE, CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr,
                        working_directory.c_str(), &startup, &info)) {
        last_error_ = GetLastError();
        release_handles();
        state_ = ProcessStatus::Failed;
        return false;
    }
    process_ = info.hProcess;
    process_id_ = info.dwProcessId;

    if (!AssignProcessToJobObject(job_, process_)) {
        last_error_ = GetLastError();
        TerminateProcess(process_, 1);
        WaitForSingleObject(process_, 5000);
        CloseHandle(info.hThread);
        release_handles();
        state_ = ProcessStatus::Failed;
        return false;
    }

    if (ResumeThread(info.hThread) == static_cast<DWORD>(-1)) {
        last_error_ = GetLastError();
        TerminateJobObject(job_, 1);
        WaitForSingleObject(process_, 5000);
        CloseHandle(info.hThread);
        release_handles();
        state_ = ProcessStatus::Failed;
        return false;
    }

    CloseHandle(info.hThread);
    state_ = ProcessStatus::Running;
    return true;
}

ProcessStatus WindowsProcess::poll() {
    if (!process_) return state_;
    const DWORD result = WaitForSingleObject(process_, 0);
    if (result == WAIT_TIMEOUT) return state_ = ProcessStatus::Running;
    if (result == WAIT_OBJECT_0) {
        release_handles();
        return state_ = ProcessStatus::Exited;
    }
    last_error_ = GetLastError();
    return state_ = ProcessStatus::Failed;
}

bool WindowsProcess::stop() {
    if (!process_) {
        release_handles();
        state_ = ProcessStatus::Stopped;
        return true;
    }
    // Deliberately force-stops the job tree. No unverified console commands.
    const BOOL terminated = TerminateJobObject(job_, 1);
    if (!terminated) last_error_ = GetLastError();
    const DWORD waited = WaitForSingleObject(process_, 5000);
    if (waited == WAIT_FAILED) last_error_ = GetLastError();
    release_handles(); // KILL_ON_JOB_CLOSE handles child processes.
    state_ = ProcessStatus::Stopped;
    return terminated && waited == WAIT_OBJECT_0;
}
}
