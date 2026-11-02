#include "../src/audio_output_preview.h"
#include <cassert>
#include <vector>
int main() {
    using namespace axv;
    const std::vector<AudioEndpoint> outputs{
        {L"headset", L"USB Headset", true, false},
        {L"speakers", L"PC Speakers", false, true}
    };
    auto endpoint=[&](const std::wstring& id,bool use_default,AudioOutputPreviewRole role) {
        return audio_output_preview_endpoint(outputs,id,use_default,role);
    };
    const auto voice=AudioOutputPreviewRole::Conversation;
    const auto ring=AudioOutputPreviewRole::Ringtone;
    assert(endpoint(L"",true,voice)==std::optional<std::wstring>{L"headset"});
    assert(endpoint(L"",true,ring)==std::optional<std::wstring>{L"speakers"});
    assert(endpoint(L"speakers",true,voice)==std::optional<std::wstring>{L"speakers"});
    assert(endpoint(L"headset",true,ring)==std::optional<std::wstring>{L"headset"});
    assert(!endpoint(L"disconnected",true,voice));
    assert(!endpoint(L"disconnected",true,ring));
    assert(!endpoint(L"",false,voice));
    assert(!endpoint(L"",false,ring));
    assert(!audio_output_preview_endpoint({},L"",true,voice));
    assert(!audio_output_preview_endpoint({},L"speakers",false,voice));
    const std::vector<AudioEndpoint> no_multimedia{
        {L"headset",L"USB Headset",true,false}
    };
    assert(!audio_output_preview_endpoint(no_multimedia,L"",true,ring));
    assert(audio_output_preview_endpoint(no_multimedia,L"",true,voice));
    const std::vector<AudioEndpoint> no_communications{
        {L"speakers",L"PC Speakers",false,true}
    };
    assert(!audio_output_preview_endpoint(no_communications,L"",true,voice));
    assert(audio_output_preview_endpoint(no_communications,L"",true,ring));
    const std::vector<AudioEndpoint> nameless{{L"",L"Unavailable",true,true}};
    assert(!audio_output_preview_endpoint(nameless,L"",true,voice));
    assert(!audio_output_preview_endpoint(nameless,L"",true,ring));
    return 0;
}
