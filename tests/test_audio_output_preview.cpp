#include "../src/audio_output_preview.h"
#include <cstdio>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr,"preview check failed on line %d: %s\n",__LINE__,#expr); return 1; } } while (false)
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
    CHECK(endpoint(L"",true,voice)==std::optional<std::wstring>{L"headset"});
    CHECK(endpoint(L"",true,ring)==std::optional<std::wstring>{L"speakers"});
    CHECK(endpoint(L"speakers",true,voice)==std::optional<std::wstring>{L"speakers"});
    CHECK(endpoint(L"headset",true,ring)==std::optional<std::wstring>{L"headset"});
    CHECK(!endpoint(L"disconnected",true,voice));
    CHECK(!endpoint(L"disconnected",true,ring));
    CHECK(!endpoint(L"",false,voice));
    CHECK(!endpoint(L"",false,ring));
    CHECK(!audio_output_preview_endpoint({},L"",true,voice));
    CHECK(!audio_output_preview_endpoint({},L"speakers",false,voice));
    const std::vector<AudioEndpoint> no_multimedia{
        {L"headset",L"USB Headset",true,false}
    };
    CHECK(!audio_output_preview_endpoint(no_multimedia,L"",true,ring));
    CHECK(audio_output_preview_endpoint(no_multimedia,L"",true,voice));
    const std::vector<AudioEndpoint> no_communications{
        {L"speakers",L"PC Speakers",false,true}
    };
    CHECK(!audio_output_preview_endpoint(no_communications,L"",true,voice));
    CHECK(audio_output_preview_endpoint(no_communications,L"",true,ring));
    const std::vector<AudioEndpoint> nameless{{L"",L"Unavailable",true,true}};
    CHECK(!audio_output_preview_endpoint(nameless,L"",true,voice));
    CHECK(!audio_output_preview_endpoint(nameless,L"",true,ring));
    return 0;
}
