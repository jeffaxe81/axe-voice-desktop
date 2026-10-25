#include "../src/baresip_probe.h"
#include <filesystem>
int main(int argc, char** argv) {
    if (argc < 1 || !argv[0]) return 1;
    const auto found = axv::probe_baresip_executable(std::filesystem::path(argv[0]));
    if (found.status != axv::BaresipProbeStatus::PresentUnverified) return 2;
    const auto missing = axv::probe_baresip_executable(std::filesystem::path("__axv_nonexistent_baresip_test_file_02901__.exe"));
    if (missing.status != axv::BaresipProbeStatus::Missing) return 3;
    const auto directory = axv::probe_baresip_executable(std::filesystem::current_path());
    if (directory.status != axv::BaresipProbeStatus::NotRegularFile) return 4;
    return 0;
}
