#include "../src/audio_role_swap.h"
#include <string>
#include <vector>

int main() {
    using namespace axv;
    const std::vector<AudioEndpoint> outputs{
        {L"headset-private-id", L"Headset", true, false},
        {L"speaker-private-id", L"PC", false, true}
    };
    const std::wstring mic = L"unchanged-mic";
    auto plan = plan_audio_role_swap(outputs, L"headset-private-id", L"speaker-private-id");
    if (!plan.can_swap || plan.next_call_index != 1 ||
        plan.next_ringtone_index != 0 || plan.next_call_id != L"speaker-private-id" ||
        plan.next_ringtone_id != L"headset-private-id") return 1;
    auto back = plan_audio_role_swap(outputs, plan.next_call_id, plan.next_ringtone_id);
    if (!back.can_swap || back.next_call_index != 0 ||
        back.next_ringtone_index != 1 || back.next_call_id != L"headset-private-id") return 2;
    if (mic != L"unchanged-mic") return 3;
    if (plan_audio_role_swap(outputs, L"headset-private-id", L"headset-private-id").can_swap) return 4;
    if (plan_audio_role_swap(outputs, L"", L"speaker-private-id").can_swap) return 5;
    if (plan_audio_role_swap(outputs, L"headset-private-id", L"").can_swap) return 6;
    if (plan_audio_role_swap(outputs, L"disconnected", L"speaker-private-id").can_swap) return 7;
    if (plan_audio_role_swap(outputs, L"headset-private-id", L"disconnected").can_swap) return 8;
    if (plan_audio_role_swap({}, L"headset-private-id", L"speaker-private-id").can_swap) return 9;
    if (plan_audio_role_swap({outputs.front()}, L"headset-private-id", L"speaker-private-id").can_swap) return 10;
    const std::vector<AudioEndpoint> duplicate{{L"", L"invalid", false, false}, outputs[1]};
    if (plan_audio_role_swap(duplicate, L"", L"speaker-private-id").can_swap) return 11;
    return 0;
}
