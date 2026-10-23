#pragma once
#include <string>
#include <vector>

namespace axv {

enum class AudioInventoryError {
    None,
    ComInitializationFailed,
    EnumeratorUnavailable,
    EndpointQueryFailed
};

struct AudioEndpoint {
    std::wstring id;     // Ephemeral Core Audio identifier.
    std::wstring name;   // Windows-provided label.
    bool default_communications = false;
    bool default_multimedia = false; // Windows eMultimedia role for ringtone preview.
};

// Deterministic hotplug fallback: keep a valid selection, else use default.
inline int preferred_audio_endpoint_index(const std::vector<AudioEndpoint>& devices,
                                           const std::wstring& previous_id) {
    if (devices.empty()) return -1;
    if (!previous_id.empty()) {
        for (std::size_t i = 0; i < devices.size(); ++i)
            if (devices[i].id == previous_id) return static_cast<int>(i);
    }
    for (std::size_t i = 0; i < devices.size(); ++i)
        if (devices[i].default_communications) return static_cast<int>(i);
    return 0;
}

struct AudioInventory {
    unsigned int input_count = 0;
    unsigned int output_count = 0;
    bool has_default_communications_input = false;
    bool has_default_communications_output = false;
    std::vector<AudioEndpoint> input_devices;
    std::vector<AudioEndpoint> output_devices;
    AudioInventoryError error = AudioInventoryError::None;
};

// Endpoint names and IDs are in memory only. This does not route Baresip audio.
// Inventory does NOT confirm microphone permissions, audio I/O or SIP/RTP.
AudioInventory inspect_windows_audio_devices();

inline std::wstring audio_inventory_summary(const AudioInventory& inventory) {
    if (inventory.error != AudioInventoryError::None) {
        return L"Falha ao consultar dispositivos de áudio do Windows.";
    }
    if (inventory.input_count == 0 && inventory.output_count == 0) {
        return L"Nenhum microfone ou saída de áudio ativa detectada.";
    }
    return L"Áudio: " + std::to_wstring(inventory.input_count) + L" entrada(s), " +
        std::to_wstring(inventory.output_count) + L" saída(s). Padrão de comunicação: " +
        (inventory.has_default_communications_input ? L"microfone OK" : L"microfone ausente") +
        L"; " +
        (inventory.has_default_communications_output ? L"saída OK" : L"saída ausente") +
        L". Apenas inventário; áudio não testado.";
}

} // namespace axv
