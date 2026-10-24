#include "../src/audio_test_tone.h"
#include "../src/audio_test_tone_win32.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

int main() {
    using namespace axv;
    constexpr ToneFormat float_stereo{48000, 2, ToneSampleType::Float32};
    const auto n = tone_total_frames(float_stereo);
    if (n != 12000 || !tone_format_valid(float_stereo)) return 1;
    std::vector<unsigned char> bytes(static_cast<std::size_t>(n) * 8, 0xcd);
    if (!write_tone_packet(bytes.data(), bytes.size(), float_stereo, 0, n)) return 2;
    float first = -1, last = -1;
    std::memcpy(&first, bytes.data(), 4);
    std::memcpy(&last, bytes.data() + (n-1u)*8u, 4);
    if (first != 0.0f || last != 0.0f) return 3;
    bool audible = false;
    for (std::uint32_t i = 0; i < n; ++i) {
        float left = 0, right = 0;
        std::memcpy(&left, bytes.data() + i*8u, 4);
        std::memcpy(&right, bytes.data() + i*8u+4u, 4);
        if (left != right || !std::isfinite(left) || std::abs(left)>0.081f) return 4;
        if (std::abs(left)>0.03f) audible = true;
    }
    if (!audible) return 5;
    // A partial streaming packet must equal the matching region of a single
    // complete write; phase and ramps are tied to absolute sample position.
    std::vector<unsigned char> partial(500u*8u, 0);
    if (!write_tone_packet(partial.data(), partial.size(), float_stereo, 2900, 500)) return 6;
    if (!std::equal(partial.begin(), partial.end(), bytes.begin()+2900u*8u)) return 7;
    if (write_tone_packet(bytes.data(), 10, float_stereo, 0, n) ||
        write_tone_packet(bytes.data(), bytes.size(), float_stereo, n+1, 1) ||
        write_tone_packet(nullptr, bytes.size(), float_stereo, 0, n) ||
        write_tone_packet(bytes.data(), bytes.size(), float_stereo, n, 1)) return 8;

    for (ToneSampleType type : {ToneSampleType::Pcm16, ToneSampleType::Pcm32}) {
        const ToneFormat fmt{44100, 1, type};
        const auto count = tone_total_frames(fmt);
        const auto size = tone_sample_bytes(type);
        std::vector<unsigned char> pcm(static_cast<std::size_t>(count)*size);
        if (!write_tone_packet(pcm.data(),pcm.size(),fmt,0,count)) return 9;
        if (type == ToneSampleType::Pcm16) {
            std::int16_t first_pcm = -1, middle = 0, last_pcm = -1;
            std::memcpy(&first_pcm,pcm.data(),2);
            std::memcpy(&middle,pcm.data()+123u*size,2);
            std::memcpy(&last_pcm,pcm.data()+(count-1u)*size,2);
            if (first_pcm || last_pcm || !middle) return 10;
        } else {
            std::int32_t first_pcm = -1, middle = 0, last_pcm = -1;
            std::memcpy(&first_pcm,pcm.data(),4);
            std::memcpy(&middle,pcm.data()+123u*size,4);
            std::memcpy(&last_pcm,pcm.data()+(count-1u)*size,4);
            if (first_pcm || last_pcm || !middle) return 11;
        }
    }
    if (tone_total_frames({0,2,ToneSampleType::Float32}) != 0 ||
        tone_format_valid({192001,2,ToneSampleType::Float32}) ||
        tone_format_valid({48000,0,ToneSampleType::Float32}) ||
        tone_format_valid({48000,9,ToneSampleType::Float32})) return 12;

    // No hardware-dependent audio playback is required on GitHub runners.
    if (play_windows_tone_on_endpoint(L"") != TonePreviewState::MissingDevice)
        return 13;
    if (play_windows_tone_on_endpoint(std::wstring(5000,L'x'))
            != TonePreviewState::MissingDevice) return 14;
    if (std::wstring_view(tone_preview_feedback(TonePreviewState::Played))
            .find(L"Sem chamada SIP") == std::wstring_view::npos) return 15;
    if (std::wstring_view(tone_preview_feedback(TonePreviewState::MissingDevice))
            .find(L"desconectada") == std::wstring_view::npos) return 16;
    return 0;
}
