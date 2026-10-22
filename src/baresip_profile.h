#pragma once
#include <filesystem>
#include <fstream>
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
    InsecureRemoteControl,
    ReadFailed
};

// Laboratory profile only. This does not provide credential storage or a secure IPC channel.
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
        std::ifstream input(file, std::ios::binary);
        if (!input.is_open()) return BaresipProfileStatus::ReadFailed;
        std::string line;
        while (std::getline(input, line)) {
            const auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos || line[start] == '#') continue;
            if (std::string(filename) == "accounts")
                return BaresipProfileStatus::AccountsConfigured;
            // No unauthenticated control listener in a laboratory process.
            const std::string active = line.substr(start);
            if (active.find("ctrl_tcp") != std::string::npos ||
                active.find("ctrl_http") != std::string::npos ||
                active.find("ctrl_ws") != std::string::npos)
                return BaresipProfileStatus::InsecureRemoteControl;
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
    case BaresipProfileStatus::InsecureRemoteControl: return L"Controle TCP/HTTP não autenticado detectado: execução bloqueada.";
    case BaresipProfileStatus::ReadFailed: return L"Não foi possível inspecionar o perfil Baresip.";
    }
    return L"Erro ao validar o perfil Baresip.";
}
}
