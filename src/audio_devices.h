#pragma once
#include <string>

namespace axv {

enum class AudioInventoryError {
    None,
    ComInitializationFailed,
    EnumeratorUnavailable,
    EndpointQueryFailed
};

struct AudioInventory {
    unsigned int input_count = 0;
    unsigned int output_count = 0;
    bool has_default_communications_input = false;
    bool has_default_communications_output = false;
    AudioInventoryError error = AudioInventoryError::None;
};

// Inventory only. It does NOT confirm microphone permissions, audio I/O or SIP/RTP.
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
