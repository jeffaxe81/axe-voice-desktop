#include "../src/baresip_modules.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <windows.h>

int main() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() /
        (L"AXE Voice modules check " + std::to_wstring(GetCurrentProcessId()));
    std::error_code ec;
    fs::remove_all(root, ec);
    if (!fs::create_directories(root, ec) || ec) return 1;
    const auto exe = root / L"baresip.exe";
    auto audit = axv::inspect_baresip_module_files(exe);
    if (audit.status != axv::BaresipModuleScanStatus::MissingBinary) return 2;
    {
        std::ofstream f(exe);
        f << "not a real executable";
    }
    audit = axv::inspect_baresip_module_files(exe);
    if (audit.status != axv::BaresipModuleScanStatus::MissingModuleDirectory) return 3;
    const auto modules = root / L"modules";
    if (!fs::create_directory(modules, ec) || ec) return 4;
    for (const auto* name : {L"g711.dll", L"opus.dll", L"wasapi.dll"}) {
        std::ofstream f(modules / name);
        f << "stub";
    }
    audit = axv::inspect_baresip_module_files(exe);
    if (audit.status != axv::BaresipModuleScanStatus::Inspected) return 5;
    if (!audit.g711_file || !audit.opus_file || !audit.wasapi_file) return 6;
    if (audit.g722_file || audit.winwave_file || audit.srtp_file || audit.dtls_srtp_file) return 7;
    if (axv::baresip_module_inventory_summary(audit).find(L"G.711") == std::wstring::npos) return 8;
    fs::remove(modules / L"opus.dll", ec);
    fs::create_directory(modules / L"opus.dll", ec);
    audit = axv::inspect_baresip_module_files(exe);
    if (audit.status != axv::BaresipModuleScanStatus::UnsafeModuleDirectory) return 9;
    fs::remove_all(root, ec);
    return ec ? 10 : 0;
}
