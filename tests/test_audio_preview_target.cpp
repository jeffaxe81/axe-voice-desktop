#include "../src/audio_preview_target.h"
#include <string>
#include <vector>
int main() {
    using namespace axv;
    const std::vector<AudioEndpoint> outputs{
        {L"headset-private", L"Headset", true, false},
        {L"speaker-private", L"Alto falante", false, true}
    };
    auto voice = select_audio_preview_target(outputs, L"headset-private",
        L"speaker-private", AudioPreviewRole::Voice);
    auto ring = select_audio_preview_target(outputs, L"headset-private",
        L"speaker-private", AudioPreviewRole::Ringtone);
    if (!voice.available || voice.endpoint_id != L"headset-private") return 1;
    if (!ring.available || ring.endpoint_id != L"speaker-private") return 2;
    voice = select_audio_preview_target(outputs, L"speaker-private",
        L"headset-private", AudioPreviewRole::Voice);
    ring = select_audio_preview_target(outputs, L"speaker-private",
        L"headset-private", AudioPreviewRole::Ringtone);
    if (!voice.available || voice.endpoint_id != L"speaker-private") return 3;
    if (!ring.available || ring.endpoint_id != L"headset-private") return 4;
    if (voice.endpoint_id == ring.endpoint_id) return 5;
    // Shared output is valid, but never becomes a second physical device.
    voice = select_audio_preview_target(outputs, L"headset-private",
        L"headset-private", AudioPreviewRole::Voice);
    ring = select_audio_preview_target(outputs, L"headset-private",
        L"headset-private", AudioPreviewRole::Ringtone);
    if (!voice.available || !ring.available ||
        voice.endpoint_id != ring.endpoint_id) return 6;
    // Missing/removed endpoint cannot fall back to the other role.
    if (select_audio_preview_target(outputs, L"unplugged", L"speaker-private",
        AudioPreviewRole::Voice).available) return 7;
    if (select_audio_preview_target(outputs, L"headset-private", L"unplugged",
        AudioPreviewRole::Ringtone).available) return 8;
    if (select_audio_preview_target(outputs, L"", L"speaker-private",
        AudioPreviewRole::Voice).available) return 9;
    if (select_audio_preview_target(outputs, L"headset-private", L"",
        AudioPreviewRole::Ringtone).available) return 10;
    if (select_audio_preview_target({}, L"headset-private", L"speaker-private",
        AudioPreviewRole::Ringtone).available) return 11;
    if (select_audio_preview_target(outputs, std::wstring(4097,L'x'), L"speaker-private",
        AudioPreviewRole::Voice).available) return 12;
    if (select_audio_preview_target({{L"", L"invalid", false,false}},
        L"", L"", AudioPreviewRole::Ringtone).available) return 13;
    return 0;
}
