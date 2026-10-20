#include "../src/audio_devices.h"
#include <string>

int main() {
    axv::AudioInventory none;
    if (axv::audio_inventory_summary(none).find(L"Nenhum") == std::wstring::npos) return 1;

    axv::AudioInventory detected;
    detected.input_count = 1;
    detected.output_count = 2;
    detected.has_default_communications_input = true;
    detected.has_default_communications_output = true;
    const std::wstring text = axv::audio_inventory_summary(detected);
    if (text.find(L"1 entrada(s)") == std::wstring::npos) return 2;
    if (text.find(L"2 saída(s)") == std::wstring::npos) return 3;
    if (text.find(L"áudio não testado") == std::wstring::npos) return 4;

    detected.error = axv::AudioInventoryError::EndpointQueryFailed;
    if (axv::audio_inventory_summary(detected).find(L"Falha") == std::wstring::npos) return 5;
    return 0;
}
