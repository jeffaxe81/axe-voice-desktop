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
    if (axv::inspect_baresip_config_line("module_app CTRL_TCP.SO")
        != axv::BaresipProfileStatus::InsecureRemoteControl) return 11;
    if (axv::inspect_baresip_config_line("# CTRL_HTTP.so")
        != axv::BaresipProfileStatus::Ready) return 12;
    if (axv::inspect_baresip_config_line("include /tmp/other.conf")
        != axv::BaresipProfileStatus::UnsafeConfiguration) return 13;
    if (axv::inspect_baresip_config_line("module_path C:/other")
        != axv::BaresipProfileStatus::UnsafeConfiguration) return 14;
    if (axv::inspect_baresip_config_line("module G711.SO")
        != axv::BaresipProfileStatus::Ready) return 15;
    {
        std::ofstream config(base / "config");
        config << "module_app CTRL_WS.so\n";
    }
    if (axv::prepare_baresip_lab_profile(base)
        != axv::BaresipProfileStatus::InsecureRemoteControl) return 16;
    {
        std::ofstream config(base / "config");
        config << "module_path C:/unsafe/modules\n";
    }
    if (axv::prepare_baresip_lab_profile(base)
        != axv::BaresipProfileStatus::UnsafeConfiguration) return 17;
    {
        std::ofstream config(base / "config");
        config << std::string(128 * 1024 + 1, 'x');
    }
    if (axv::prepare_baresip_lab_profile(base)
        != axv::BaresipProfileStatus::OversizedConfiguration) return 18;
    {
        std::ofstream config(base / "config");
        config << std::string(4100, 'x');
    }
    if (axv::prepare_baresip_lab_profile(base)
        != axv::BaresipProfileStatus::UnsafeConfiguration) return 19;
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
