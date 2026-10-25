#include <windows.h>
#include <filesystem>
#include <fstream>
int wmain(int argc, wchar_t** argv) {
    // Inert test child. When launched with -f, leave an argv marker for the parent test.
    if (argc == 3) {
        if (wcscmp(argv[1], L"-f") != 0) return 2;
        const std::filesystem::path profile(argv[2]);
        if (!std::filesystem::is_directory(profile)) return 3;
        std::ofstream marker(profile / "axe-voice-test-child-started");
        if (!marker) return 4;
        marker << "ok";
    } else if (argc != 1) {
        return 5;
    }
    Sleep(30000);
    return 0;
}
