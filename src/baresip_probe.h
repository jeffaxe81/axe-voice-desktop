#pragma once
#include <filesystem>
#include <string>
namespace axv {
enum class BaresipProbeStatus { Missing, NotRegularFile, PresentUnverified };
struct BaresipProbe {
    BaresipProbeStatus status;
    std::wstring message;
};
inline BaresipProbe probe_baresip_executable(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::exists(path, error) || error)
        return {BaresipProbeStatus::Missing, L"Baresip não encontrado no caminho informado."};
    if (!std::filesystem::is_regular_file(path, error) || error)
        return {BaresipProbeStatus::NotRegularFile, L"O caminho informado não é um arquivo regular."};
    return {BaresipProbeStatus::PresentUnverified,
        L"Arquivo encontrado. Compatibilidade, assinatura e integração SIP não verificadas."};
}
}
