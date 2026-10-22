#include "../src/audio_hotplug_refresh.h"

int main() {
    using namespace axv;
    AudioInventory initial;
    if (audio_inventory_changed(initial, initial)) return 1;
    AudioInventory first = initial;
    first.input_devices = {{L"mic1", L"USB microphone", true}};
    first.output_devices = {{L"speaker1", L"Headset", true}};
    first.input_count = first.output_count = 1;
    first.has_default_communications_input = true;
    first.has_default_communications_output = true;
    if (!audio_inventory_changed(initial, first)) return 2;
    if (audio_inventory_changed(first, first)) return 3;
    AudioInventory change = first;
    change.input_devices[0].default_communications = false;
    if (!audio_inventory_changed(first, change)) return 4;
    change = first;
    change.input_devices[0].name = L"Renamed microphone";
    if (!audio_inventory_changed(first, change)) return 5;
    change = first;
    change.output_devices[0].id = L"speaker2";
    if (!audio_inventory_changed(first, change)) return 6;
    change = first;
    change.output_devices.clear();
    change.output_count = 0;
    if (!audio_inventory_changed(first, change)) return 7;
    change = first;
    change.has_default_communications_input = false;
    if (!audio_inventory_changed(first, change)) return 8;
    change = first;
    change.error = AudioInventoryError::EndpointQueryFailed;
    if (!audio_inventory_changed(first, change)) return 9;
    change = first;
    change.input_devices.emplace_back(AudioEndpoint{L"mic2", L"Bluetooth", false});
    if (!audio_inventory_changed(first, change)) return 10;
    change = first;
    change.output_devices.push_back({L"speaker2", L"Secondary", false});
    if (!audio_inventory_changed(first, change)) return 11;
    return 0;
}
