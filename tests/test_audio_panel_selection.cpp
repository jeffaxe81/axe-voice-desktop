#include "../src/audio_panel_selection.h"

namespace {
bool equal(const axv::AudioPanelSelection& a, const axv::AudioPanelSelection& b) {
    return a.input_id == b.input_id && a.output_id == b.output_id &&
           a.ringtone_id == b.ringtone_id;
}
}

int main() {
    using namespace axv;
    AudioInventory inventory;
    inventory.input_devices = {{L"mic-first", L"First", false},
                               {L"mic-default", L"Default", true}};
    inventory.output_devices = {{L"speaker-first", L"First", false, false},
                                {L"call", L"Call", true, false},
                                {L"ring", L"Ring", false, true}};
    const AudioPanelSelection defaults{L"mic-default", L"call", L"ring"};
    if (!equal(reconcile_audio_panel({}, inventory, false), defaults)) return 1;

    const AudioPanelSelection explicit_choice{L"mic-first", L"ring", L"call"};
    if (!equal(reconcile_audio_panel(explicit_choice, inventory, false),
               explicit_choice)) return 2;
    const AudioPanelSelection removed{L"gone-mic", L"gone-call", L"gone-ring"};
    if (!equal(reconcile_audio_panel(removed, inventory, false), defaults)) return 3;

    // Failed snapshots, including partially populated results, preserve all IDs.
    for (const auto error : {AudioInventoryError::ComInitializationFailed,
                             AudioInventoryError::EnumeratorUnavailable,
                             AudioInventoryError::EndpointQueryFailed}) {
        inventory.error = error;
        for (const bool native : {false, true}) {
            if (!equal(reconcile_audio_panel(removed, inventory, native), removed)) return 4;
            if (!equal(reconcile_audio_panel({}, inventory, native), {})) return 5;
        }
    }
    const auto failed = reconcile_audio_panel(removed, inventory, false);
    inventory.error = AudioInventoryError::None;
    if (!equal(reconcile_audio_panel(failed, inventory, false), defaults)) return 6;

    // Native routes retain both unavailable IDs and explicit Windows defaults.
    if (!equal(reconcile_audio_panel(removed, inventory, true), removed)) return 7;
    if (!equal(reconcile_audio_panel({}, inventory, true), {})) return 8;
    const AudioPanelSelection mixed{L"", L"gone-call", L"call"};
    if (!equal(reconcile_audio_panel(mixed, inventory, true), mixed)) return 9;
    if (!equal(reconcile_audio_panel(explicit_choice, inventory, true), explicit_choice)) return 10;

    AudioInventory empty;
    if (!equal(reconcile_audio_panel(explicit_choice, empty, false), {})) return 11;
    if (!equal(reconcile_audio_panel(removed, empty, true), removed)) return 12;
    empty.error = AudioInventoryError::EndpointQueryFailed;
    if (!equal(reconcile_audio_panel(explicit_choice, empty, false), explicit_choice)) return 13;
    empty.error = AudioInventoryError::None;
    if (!equal(reconcile_audio_panel(explicit_choice, empty, false), {})) return 14;
    return 0;
}
