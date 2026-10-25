#pragma once
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

namespace axv {

enum class BaresipProfileStatus {
    Ready,
    CreateFailed,
    UnsafePath,
    UnsafeFile,
    AccountsConfigured,
    AutoAccountModule,
    InsecureRemoteControl,
    UnsafeConfiguration,
    OversizedConfiguration,
    ReadFailed
};

// Laboratory profile only. This does not provide credential storage or a secure IPC channel.
// Restrictive parsing is intentional: referenced config files and redirected module
// paths cannot be audited through the current prototype.
inline std::string baresip_lowercase_ascii(std::string input) {
    std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return input;
}
inline BaresipProfileStatus inspect_baresip_config_line(const std::string& line) {
    if (line.size() > 4096 || line.find('\0') != std::string::npos)
        return BaresipProfileStatus::UnsafeConfiguration;
    const auto start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos || line[start] == '#')
        return BaresipProfileStatus::Ready;
    std::string active = baresip_lowercase_ascii(line.substr(start));
    const auto comment = active.find('#');
    if (comment != std::string::npos) active.resize(comment);
    const auto end = active.find_first_of(" \t=\r\n");
    const std::string key = active.substr(0, end);
    if (key == "include" || key == "module_path" || key == "module_preload" ||
        key == "module_dir")
        return BaresipProfileStatus::UnsafeConfiguration;
    if (key == "module") {
        // The 'account' application module loads accounts and may register
        // automatically, bypassing the owner-thread WinCred gate.
        if (end == std::string::npos)
            return BaresipProfileStatus::UnsafeConfiguration;
        std::string value = active.substr(end);
        std::replace(value.begin(), value.end(), '=', ' ');
        std::istringstream parsed(value);
        std::string name, trailing;
        if (!(parsed >> name) || (parsed >> trailing))
            return BaresipProfileStatus::UnsafeConfiguration;
        // No module paths or quote trickery in the isolated LAB profile.
        if (name.find('/') != std::string::npos ||
            name.find('\\') != std::string::npos ||
            name.find('"') != std::string::npos ||
            name.find('\'') != std::string::npos)
            return BaresipProfileStatus::UnsafeConfiguration;
        if (name == "account" || name == "account.so" ||
            name == "account.dll" || name == "account.dylib")
            return BaresipProfileStatus::AutoAccountModule;
    }
    // Conservative: block even if the suspicious name appears in an unknown
    // directive. This is not a full Baresip configuration grammar.
    if (active.find("ctrl_tcp") != std::string::npos ||
        active.find("ctrl_http") != std::string::npos ||
        active.find("ctrl_ws") != std::string::npos)
        return BaresipProfileStatus::InsecureRemoteControl;
    return BaresipProfileStatus::Ready;
}

inline BaresipProfileStatus prepare_baresip_lab_profile(const std::filesystem::path& directory) {
    namespace fs = std::filesystem;
    if (directory.empty()) return BaresipProfileStatus::UnsafePath;
    std::error_code error;
    const auto status = fs::symlink_status(directory, error);
    if (error && error != std::errc::no_such_file_or_directory)
        return BaresipProfileStatus::UnsafePath;
    if (fs::exists(status)) {
        if (fs::is_symlink(status) || !fs::is_directory(status))
            return BaresipProfileStatus::UnsafePath;
    } else {
        error.clear();
        if (!fs::create_directories(directory, error) || error)
            return BaresipProfileStatus::CreateFailed;
    }

#ifdef _WIN32
    // Junctions and other reparse points are not necessarily reported as symlinks.
    const DWORD attributes = GetFileAttributesW(directory.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        return BaresipProfileStatus::UnsafePath;
#endif

    for (const char* filename : {"config", "accounts", "contacts", "uuid"}) {
        const fs::path file = directory / filename;
        error.clear();
        const auto file_status = fs::symlink_status(file, error);
        if (error == std::errc::no_such_file_or_directory) continue;
        if (error) return BaresipProfileStatus::ReadFailed;
        if (!fs::exists(file_status)) continue;
        if (fs::is_symlink(file_status) || !fs::is_regular_file(file_status))
            return BaresipProfileStatus::UnsafeFile;
#ifdef _WIN32
        const DWORD file_attributes = GetFileAttributesW(file.c_str());
        if (file_attributes == INVALID_FILE_ATTRIBUTES || (file_attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return BaresipProfileStatus::UnsafeFile;
#endif
        if (std::string(filename) != "accounts" && std::string(filename) != "config") continue;
        error.clear();
        const auto size = fs::file_size(file, error);
        if (error) return BaresipProfileStatus::ReadFailed;
        if (size > 128 * 1024) return BaresipProfileStatus::OversizedConfiguration;
        std::ifstream input(file, std::ios::binary);
        if (!input.is_open()) return BaresipProfileStatus::ReadFailed;
        std::string line;
        while (std::getline(input, line)) {
            if (line.size() > 4096 || line.find('\0') != std::string::npos)
                return BaresipProfileStatus::UnsafeConfiguration;
            const auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos || line[start] == '#') continue;
            if (std::string(filename) == "accounts")
                return BaresipProfileStatus::AccountsConfigured;
            const auto decision = inspect_baresip_config_line(line);
            if (decision != BaresipProfileStatus::Ready) return decision;
        }
        if (input.bad()) return BaresipProfileStatus::ReadFailed;
    }
    return BaresipProfileStatus::Ready;
}

inline const wchar_t* baresip_profile_diagnostic(BaresipProfileStatus status) {
    switch (status) {
    case BaresipProfileStatus::Ready: return L"Perfil Baresip de laboratório validado.";
    case BaresipProfileStatus::CreateFailed: return L"Falha ao criar perfil local do Baresip.";
    case BaresipProfileStatus::UnsafePath: return L"Pasta de perfil insegura ou inacessível.";
    case BaresipProfileStatus::UnsafeFile: return L"Perfil contém arquivos redirecionados ou inválidos.";
    case BaresipProfileStatus::AccountsConfigured: return L"Contas SIP já configuradas: inicialização automática bloqueada.";
    case BaresipProfileStatus::AutoAccountModule: return L"Modulo de contas automaticas Baresip bloqueado no laboratorio.";
    case BaresipProfileStatus::InsecureRemoteControl: return L"Controle remoto não auditado detectado: execução bloqueada.";
    case BaresipProfileStatus::UnsafeConfiguration: return L"Configuração Baresip não segura: include, caminho externo ou conteúdo inválido.";
    case BaresipProfileStatus::OversizedConfiguration: return L"Perfil Baresip excede o limite de leitura segura (128 KiB).";
    case BaresipProfileStatus::ReadFailed: return L"Não foi possível inspecionar o perfil Baresip.";
    }
    return L"Erro ao validar o perfil Baresip.";
}
}
