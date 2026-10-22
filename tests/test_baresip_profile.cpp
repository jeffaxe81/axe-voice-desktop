#include "../src/baresip_profile.h"
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    namespace fs = std::filesystem;
    const auto base = fs::temp_directory_path() / "AXE Voice lab profile test";
    std::error_code error;
    fs::remove_all(base, error);
    if (axv::prepare_baresip_lab_profile({}) != axv::BaresipProfileStatus::UnsafePath) return 1;
    if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::Ready) return 2;
    if (!fs::is_directory(base)) return 3;
    if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::Ready) return 4;
    {
        std::ofstream account(base / "accounts");
        account << "  # commented example\n\n";
    }
    if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::Ready) return 5;
    {
        std::ofstream account(base / "accounts", std::ios::app);
        account << "<sip:test@example.invalid>;auth_pass=not-a-real-secret\n";
    }
    if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::AccountsConfigured) return 6;
    fs::remove(base / "accounts");
    {
        std::ofstream config(base / "config");
        config << "# module_app ctrl_tcp.so\nmodule_app ctrl_tcp.so\n";
    }
    if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::InsecureRemoteControl) return 7;
    {
        std::ofstream config(base / "config");
        config << "# module_app ctrl_tcp.so\nmodule g711.so\n";
    }
    if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::Ready) return 8;
    fs::remove(base / "config");
    fs::create_directory(base / "accounts");
    if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::UnsafeFile) return 9;
    fs::remove(base / "accounts");
#ifndef _WIN32
    fs::create_directory_symlink(fs::temp_directory_path(), base / "contacts", error);
    if (!error) {
        if (axv::prepare_baresip_lab_profile(base) != axv::BaresipProfileStatus::UnsafeFile) return 10;
        fs::remove(base / "contacts");
    }
#endif
    fs::remove_all(base);
    return 0;
}
