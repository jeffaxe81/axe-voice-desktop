#include "windows_process.h"
#include <string>
#include <vector>

namespace axv {
WindowsProcess::~WindowsProcess() { stop(); }

void WindowsProcess::release_handles() {
    if (stdin_write_) { CloseHandle(stdin_write_); stdin_write_ = nullptr; }
    if (process_) { CloseHandle(process_); process_ = nullptr; }
    if (job_) { CloseHandle(job_); job_ = nullptr; }
    process_id_ = 0;
}

bool WindowsProcess::start(const std::filesystem::path& executable,
                           const std::filesystem::path& config_directory) {
    if (poll() == ProcessStatus::Running) {
        last_error_ = ERROR_ALREADY_EXISTS;
        return false;
    }
    stop();
    exit_code_.reset();
    last_error_ = ERROR_SUCCESS;

    std::error_code error;
    const auto absolute = std::filesystem::absolute(executable, error);
    if (error || !std::filesystem::is_regular_file(absolute, error) || error) {
        last_error_ = ERROR_FILE_NOT_FOUND;
        state_ = ProcessStatus::Failed;
        return false;
    }

    std::filesystem::path profile;
    if (!config_directory.empty()) {
        error.clear();
        profile = std::filesystem::absolute(config_directory, error).lexically_normal();
        if (error || !std::filesystem::is_directory(profile, error) || error ||
            std::filesystem::is_symlink(std::filesystem::symlink_status(profile, error)) || error) {
            last_error_ = ERROR_INVALID_PARAMETER;
            state_ = ProcessStatus::Failed;
            return false;
        }
        const DWORD attrs = GetFileAttributesW(profile.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) {
            last_error_ = ERROR_INVALID_PARAMETER;
            state_ = ProcessStatus::Failed;
            return false;
        }
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

    // Explicit lpApplicationName avoids PATH lookup. Quoted paths permit spaces and Unicode.
    // Config-directory is an actual directory and Windows path components cannot contain quotes.
    // Launch a one-way stdin pipe; only the fixed /quit command may be sent.
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE child_read = nullptr, parent_write = nullptr;
    if (!CreatePipe(&child_read, &parent_write, &sa, 0)) {
        last_error_ = GetLastError();
        release_handles(); state_ = ProcessStatus::Failed; return false;
    }
    if (!SetHandleInformation(parent_write, HANDLE_FLAG_INHERIT, 0)) {
        last_error_ = GetLastError();
        CloseHandle(child_read); CloseHandle(parent_write);
        release_handles(); state_ = ProcessStatus::Failed; return false;
    }
    HANDLE nul = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nul == INVALID_HANDLE_VALUE) {
        last_error_ = GetLastError();
        CloseHandle(child_read); CloseHandle(parent_write);
        release_handles(); state_ = ProcessStatus::Failed; return false;
    }
    SIZE_T attr_size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_size);
    std::vector<unsigned char> attr_storage(attr_size);
    auto* attrs = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attr_storage.data());
    if (!InitializeProcThreadAttributeList(attrs, 1, 0, &attr_size)) {
        last_error_ = GetLastError();
        CloseHandle(child_read); CloseHandle(parent_write); CloseHandle(nul);
        release_handles(); state_ = ProcessStatus::Failed; return false;
    }
    HANDLE allowed[] = {child_read, nul};
    if (!UpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   allowed, sizeof(allowed), nullptr, nullptr)) {
        last_error_ = GetLastError();
        DeleteProcThreadAttributeList(attrs);
        CloseHandle(child_read); CloseHandle(parent_write); CloseHandle(nul);
        release_handles(); state_ = ProcessStatus::Failed; return false;
    }

    std::wstring command = L"\"" + absolute.wstring() + L"\"";
    if (!profile.empty()) command += L" -f \"" + profile.wstring() + L"\"";
    std::vector<wchar_t> command_buffer(command.begin(), command.end());
    command_buffer.push_back(L'\0');
    const auto working_directory = absolute.parent_path().wstring();
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = child_read;
    startup.StartupInfo.hStdOutput = nul;
    startup.StartupInfo.hStdError = nul;
    startup.lpAttributeList = attrs;
    PROCESS_INFORMATION info = {};
    const BOOL created = CreateProcessW(absolute.c_str(), command_buffer.data(),
        nullptr, nullptr, TRUE, CREATE_SUSPENDED | CREATE_NO_WINDOW |
        EXTENDED_STARTUPINFO_PRESENT, nullptr, working_directory.c_str(),
        &startup.StartupInfo, &info);
    if (!created) last_error_ = GetLastError();
    DeleteProcThreadAttributeList(attrs);
    CloseHandle(child_read);
    CloseHandle(nul);
    if (!created) {
        CloseHandle(parent_write);
        release_handles(); state_ = ProcessStatus::Failed; return false;
    }
    stdin_write_ = parent_write;
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
        DWORD code = 0;
        if (GetExitCodeProcess(process_, &code)) {
            exit_code_ = code;
        } else {
            last_error_ = GetLastError();
            exit_code_.reset();
        }
        release_handles();
        return state_ = ProcessStatus::Exited;
    }
    last_error_ = GetLastError();
    return state_ = ProcessStatus::Failed;
}

GracefulStopResult WindowsProcess::stop_gracefully(DWORD timeout_ms) {
    if (!process_) { stop(); return GracefulStopResult::AlreadyStopped; }
    constexpr char command[] = "/quit\n";
    if (stdin_write_) {
        DWORD bytes = 0;
        const BOOL sent = WriteFile(stdin_write_, command, sizeof(command)-1, &bytes, nullptr);
        if (!sent) last_error_ = GetLastError();
        else if (bytes != sizeof(command)-1) last_error_ = ERROR_WRITE_FAULT;
        CloseHandle(stdin_write_); stdin_write_ = nullptr;
    }
    const DWORD waited = WaitForSingleObject(process_, timeout_ms);
    if (waited == WAIT_OBJECT_0) {
        DWORD code = 0;
        if (GetExitCodeProcess(process_, &code)) exit_code_ = code;
        else { last_error_ = GetLastError(); exit_code_.reset(); }
        release_handles(); state_ = ProcessStatus::Stopped;
        return GracefulStopResult::ExitedAfterQuit;
    }
    if (waited == WAIT_FAILED) last_error_ = GetLastError();
    const BOOL killed = TerminateJobObject(job_, 1);
    if (!killed) last_error_ = GetLastError();
    const DWORD done = WaitForSingleObject(process_, 5000);
    if (done == WAIT_FAILED) last_error_ = GetLastError();
    exit_code_.reset(); release_handles();
    state_ = killed && done == WAIT_OBJECT_0 ? ProcessStatus::Stopped : ProcessStatus::Failed;
    return state_ == ProcessStatus::Stopped ? GracefulStopResult::Forced : GracefulStopResult::Failed;
}

bool WindowsProcess::stop() {
    exit_code_.reset(); // Intentional stops are not natural process exit codes.
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
