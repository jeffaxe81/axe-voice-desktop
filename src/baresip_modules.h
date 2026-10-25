#pragma once
#include <filesystem>
#include <string>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

namespace axv {

// Filesystem audit only: file presence does NOT imply codec availability,
// successful DLL loading, a trustworthy publisher, or a working SIP call.
enum class LocalArtifact { Missing, Present, Unsafe };
enum class BaresipModuleScanStatus {
    MissingBinary, UnsafeBinary, MissingModuleDirectory,
    UnsafeModuleDirectory, Inspected
};

inline LocalArtifact inspect_local_artifact(const std::filesystem::path& path,
                                           bool expect_directory) {
    namespace fs = std::filesystem;
    if (path.empty()) return LocalArtifact::Missing;
    std::error_code ec;
    const auto status = fs::symlink_status(path, ec);
    if (ec == std::errc::no_such_file_or_directory)
        return LocalArtifact::Missing;
    if (ec || fs::is_symlink(status)) return LocalArtifact::Unsafe;
    if (expect_directory ? !fs::is_directory(status) : !fs::is_regular_file(status))
        return LocalArtifact::Unsafe;
#ifdef _WIN32
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_REPARSE_POINT))
        return LocalArtifact::Unsafe;
#endif
    return LocalArtifact::Present;
}

struct BaresipModuleInventory {
    BaresipModuleScanStatus status = BaresipModuleScanStatus::MissingBinary;
    bool g711_file = false;
    bool g722_file = false;
    bool opus_file = false;
    bool wasapi_file = false;
    bool winwave_file = false;
    bool srtp_file = false;
    bool dtls_srtp_file = false;
    // Not a codec negotiation or load test; G.729 stays disabled.
};

inline BaresipModuleInventory inspect_baresip_module_files(
        const std::filesystem::path& executable) {
    namespace fs = std::filesystem;
    BaresipModuleInventory result;
    const auto binary = inspect_local_artifact(executable, false);
    if (binary == LocalArtifact::Missing) return result;
    if (binary == LocalArtifact::Unsafe) {
        result.status = BaresipModuleScanStatus::UnsafeBinary;
        return result;
    }
    // This diagnostic intentionally inspects just one deterministic Windows layout.
    // Other layouts and statically linked codecs are reported as UNKNOWN.
    const fs::path modules = executable.parent_path() / L"modules";
    const auto directory = inspect_local_artifact(modules, true);
    if (directory == LocalArtifact::Missing) {
        result.status = BaresipModuleScanStatus::MissingModuleDirectory;
        return result;
    }
    if (directory == LocalArtifact::Unsafe) {
        result.status = BaresipModuleScanStatus::UnsafeModuleDirectory;
        return result;
    }

    struct Probe { const wchar_t* file; bool* target; };
    const Probe probes[] = {
        {L"g711.dll", &result.g711_file},
        {L"g722.dll", &result.g722_file},
        {L"opus.dll", &result.opus_file},
        {L"wasapi.dll", &result.wasapi_file},
        {L"winwave.dll", &result.winwave_file},
        {L"srtp.dll", &result.srtp_file},
        {L"dtls_srtp.dll", &result.dtls_srtp_file}
    };
    for (const auto& probe : probes) {
        const auto result_file = inspect_local_artifact(modules / probe.file, false);
        if (result_file == LocalArtifact::Unsafe) {
            result.status = BaresipModuleScanStatus::UnsafeModuleDirectory;
            return result;
        }
        *probe.target = result_file == LocalArtifact::Present;
    }
    result.status = BaresipModuleScanStatus::Inspected;
    return result;
}

inline std::wstring baresip_module_inventory_summary(const BaresipModuleInventory& i) {
    switch (i.status) {
    case BaresipModuleScanStatus::MissingBinary:
        return L"baresip.exe nao foi encontrado ao lado do AXE Voice.";
    case BaresipModuleScanStatus::UnsafeBinary:
        return L"baresip.exe nao e um arquivo regular ou possui redirecionamento. Inicializacao bloqueada.";
    case BaresipModuleScanStatus::UnsafeModuleDirectory:
        return L"Pasta modules ou arquivo examinado possui tipo/redirecionamento inseguro. Inicializacao bloqueada.";
    case BaresipModuleScanStatus::MissingModuleDirectory:
        return L"Arquivo Baresip encontrado. Pasta modules ausente.\n"
               L"Codecs estaticos ou instalacao em outro layout: DESCONHECIDOS.";
    case BaresipModuleScanStatus::Inspected:
        break;
    }
    std::wstring out = L"Inventario de arquivos (NAO comprova suporte SIP/codec):\n";
    const auto append = [&out](const wchar_t* label, bool found) {
        out += label;
        out += found ? L": arquivo presente\n" : L": ausente\n";
    };
    append(L"G.711 (PCMA/PCMU)", i.g711_file);
    append(L"G.722", i.g722_file);
    append(L"Opus", i.opus_file);
    append(L"WASAPI", i.wasapi_file);
    append(L"WinWave", i.winwave_file);
    append(L"SRTP", i.srtp_file);
    append(L"DTLS-SRTP", i.dtls_srtp_file);
    out += L"G.729: indisponivel ate verificacao independente.\n"
           L"Arquivos presentes nao foram carregados ou autenticados.\n"
           L"Registro e chamadas SIP continuam desabilitados.";
    return out;
}
}
