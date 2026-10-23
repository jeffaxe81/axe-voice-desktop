#include "../src/audio_preview_tone.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

int main() {
    using namespace axv;
    if (preview_tone_sample(0, 48000) != 0.0f ||
        preview_tone_sample(48000,48000) != 0.0f ||
        preview_tone_sample(1,0) != 0.0f) return 1;
    std::array<float,96000> stereo{};
    if (!fill_preview_tone(stereo.data(),48000,2,48000,ToneSampleEncoding::Float32))
        return 2;
    float peak = 0.0f;
    for (std::size_t i=0;i<stereo.size();i+=2) {
        if (stereo[i] != stereo[i+1]) return 3;
        peak = std::max(peak,std::fabs(stereo[i]));
    }
    if (peak < 0.05f || peak > 0.111f) return 4;
    if (stereo[0] != 0.0f || stereo[stereo.size()-1] != 0.0f) return 5;
    std::array<std::int16_t,9600> mono{};
    if (!fill_preview_tone(mono.data(),9600,1,48000,ToneSampleEncoding::Signed16))
        return 6;
    int peak16=0;
    for(auto x:mono) peak16=std::max(peak16,std::abs(static_cast<int>(x)));
    if (peak16 < 1500 || peak16 > 3700) return 7;
    if (fill_preview_tone(nullptr,16,1,48000,ToneSampleEncoding::Float32) ||
        fill_preview_tone(stereo.data(),16,0,48000,ToneSampleEncoding::Float32) ||
        fill_preview_tone(stereo.data(),16,2,0,ToneSampleEncoding::Float32) ||
        fill_preview_tone(stereo.data(),384001,2,48000,ToneSampleEncoding::Float32))
        return 8;
    return 0;
}
