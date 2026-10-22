#include "../src/audio_panel_refresh.h"
#include <string>

int main() {
    using namespace axv;
    AudioInventory initial;
    initial.input_devices = {{L"mic-usb", L"USB", false}, {L"mic-default", L"Default", true}};
    initial.output_devices = {{L"speaker-usb", L"USB", false}, {L"speaker-default", L"Default", true}};
    auto selected = plan_audio_panel_refresh(initial, L"mic-usb", L"speaker-usb");
    if (!selected.scan_ok || selected.microphone.selected_id != L"mic-usb" ||
        selected.speaker.selected_id != L"speaker-usb" || selected.microphone.replaced_missing_device) return 1;

    AudioInventory temporary_failure;
    temporary_failure.error = AudioInventoryError::EndpointQueryFailed;
    auto failed = plan_audio_panel_refresh(temporary_failure, selected.microphone.selected_id,
                                           selected.speaker.selected_id);
    if (failed.scan_ok || failed.microphone.available() || failed.speaker.available() ||
        failed.microphone.selected_id != L"mic-usb" || failed.speaker.selected_id != L"speaker-usb" ||
        failed.microphone.replaced_missing_device || failed.speaker.replaced_missing_device) return 2;

    auto recovered = plan_audio_panel_refresh(initial, failed.microphone.selected_id, failed.speaker.selected_id);
    if (!recovered.scan_ok || recovered.microphone.selected_id != L"mic-usb" ||
        recovered.speaker.selected_id != L"speaker-usb") return 3;

    AudioInventory unplugged = initial;
    unplugged.input_devices.erase(unplugged.input_devices.begin());
    unplugged.output_devices.erase(unplugged.output_devices.begin());
    auto fallback = plan_audio_panel_refresh(unplugged, recovered.microphone.selected_id,
                                              recovered.speaker.selected_id);
    if (!fallback.scan_ok || fallback.microphone.selected_id != L"mic-default" ||
        fallback.speaker.selected_id != L"speaker-default" ||
        !fallback.microphone.replaced_missing_device || !fallback.speaker.replaced_missing_device) return 4;

    AudioInventory no_devices;
    auto empty = plan_audio_panel_refresh(no_devices, L"mic-usb", L"speaker-usb");
    if (!empty.scan_ok || empty.microphone.available() || empty.speaker.available() ||
        !empty.microphone.selected_id.empty() || !empty.speaker.selected_id.empty()) return 5;

    return 0;
}
