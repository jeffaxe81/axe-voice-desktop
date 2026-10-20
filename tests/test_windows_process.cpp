#include "../src/windows_process.h"
#include <filesystem>
#include <iostream>

int wmain(int argc, wchar_t** argv) {
    using namespace axv;
    if (argc != 2) return 1;
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
    return 0;
}
