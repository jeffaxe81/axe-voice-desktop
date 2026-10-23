#pragma once
#include "audio_devices.h"
#include <string>
#include <vector>

namespace axv {
// UI-only plan: exchanging endpoints never starts sound, registration or SIP.
struct AudioRoleSwapPlan {
    bool can_swap = false;
    int next_call_index = -1;
    int next_ringtone_index = -1;
    std::wstring next_call_id;
    std::wstring next_ringtone_id;
};

inline AudioRoleSwapPlan plan_audio_role_swap(
    const std::vector<AudioEndpoint>& outputs,
    const std::wstring& call_id, const std::wstring& ringtone_id) {
    if (call_id.empty() || ringtone_id.empty() || call_id == ringtone_id)
        return {};
    int call_index = -1;
    int ringtone_index = -1;
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        if (outputs[i].id.empty()) continue;
        if (outputs[i].id == call_id) call_index = static_cast<int>(i);
        if (outputs[i].id == ringtone_id) ringtone_index = static_cast<int>(i);
    }
    if (call_index < 0 || ringtone_index < 0 || call_index == ringtone_index)
        return {};
    return {true, ringtone_index, call_index, ringtone_id, call_id};
}

} // namespace axv
