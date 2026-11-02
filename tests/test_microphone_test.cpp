#include "../src/microphone_test.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr,"microphone test line %d: %s\n", __LINE__, #test); return 1; } } while (false)

int main() {
    using namespace axv;
    std::vector<AudioEndpoint> inputs{
        {L"headset", L"USB headset", true, false},
        {L"desk", L"Desk microphone", false, false}
    };
    CHECK(microphone_test_endpoint(inputs,L"",true) == std::optional<std::wstring>{L"headset"});
    CHECK(microphone_test_endpoint(inputs,L"desk",true) == std::optional<std::wstring>{L"desk"});
    CHECK(!microphone_test_endpoint(inputs,L"removed",true));
    CHECK(!microphone_test_endpoint(inputs,L"",false));
    CHECK(!microphone_test_endpoint({},L"",true));
    CHECK(!microphone_test_endpoint({{L"",L"Missing",true,false}},L"",true));
    CHECK(!microphone_test_endpoint({{L"desk",L"Desk",false,false}},L"",true));

    const MicrophoneSampleFormat float_format{48000,1,MicrophoneSampleType::Float32};
    const std::array<float,4> quiet{{0.0f,0.001f,-0.003f,0.0f}};
    float peak = -1;
    CHECK(microphone_packet_peak(reinterpret_cast<const unsigned char*>(quiet.data()),
        sizeof(quiet),float_format,4,peak) && peak > 0.002f && peak < 0.004f);
    const std::array<float,4> voice{{0.2f,-0.6f,0.4f,0.1f}};
    CHECK(microphone_packet_peak(reinterpret_cast<const unsigned char*>(voice.data()),
        sizeof(voice),float_format,4,peak) && peak > 0.59f && peak < 0.61f);
    CHECK(!microphone_packet_peak(reinterpret_cast<const unsigned char*>(voice.data()),
        sizeof(voice)-1,float_format,4,peak));
    CHECK(!microphone_packet_peak(nullptr,100,float_format,4,peak));
    CHECK(!microphone_packet_peak(reinterpret_cast<const unsigned char*>(voice.data()),
        sizeof(voice),float_format,0,peak));
    CHECK(!microphone_packet_peak(reinterpret_cast<const unsigned char*>(voice.data()),
        sizeof(voice),{48000,0,MicrophoneSampleType::Float32},4,peak));

    const std::array<float,2> invalid{{0.1f,std::numeric_limits<float>::quiet_NaN()}};
    CHECK(!microphone_packet_peak(reinterpret_cast<const unsigned char*>(invalid.data()),
        sizeof(invalid),float_format,2,peak));
    const std::array<std::int16_t,3> pcm16{{-32768,0,16384}};
    CHECK(microphone_packet_peak(reinterpret_cast<const unsigned char*>(pcm16.data()),
        sizeof(pcm16),{16000,1,MicrophoneSampleType::Pcm16},3,peak) && peak==1.0f);
    const std::array<std::int32_t,2> pcm32{{0,-2147483647-1}};
    CHECK(microphone_packet_peak(reinterpret_cast<const unsigned char*>(pcm32.data()),
        sizeof(pcm32),{48000,1,MicrophoneSampleType::Pcm32},2,peak) && peak==1.0f);
    return 0;
}
