#pragma once
#include "audio_preview_tone.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace axv {
// Percent of the quiet preview signal, NOT the Windows master volume or SIP gain.
inline constexpr std::array<std::uint8_t,4> kPreviewLevels{{25,50,75,100}};
inline constexpr std::uint8_t kPreviewDefaultLevel = 75;
inline int preview_level_index(std::uint8_t level) {
    for (std::size_t i=0;i<kPreviewLevels.size();++i)
        if (kPreviewLevels[i]==level) return static_cast<int>(i);
    return -1;
}
inline std::uint8_t preview_level_for_index(int index,std::uint8_t fallback) {
    if (index<0 || static_cast<std::size_t>(index)>=kPreviewLevels.size())
        return fallback;
    return kPreviewLevels[static_cast<std::size_t>(index)];
}
inline bool scale_preview_buffer(void* buffer,std::uint32_t frames,
        std::uint16_t channels,ToneSampleEncoding encoding,std::uint8_t level) {
    if(!buffer || !frames || frames>384000 || !channels || channels>8 || level>100)
        return false;
    const std::size_t samples=static_cast<std::size_t>(frames)*channels;
    const float gain=static_cast<float>(level)/100.0f;
    if(encoding==ToneSampleEncoding::Float32) {
        auto* data=static_cast<float*>(buffer);
        for(std::size_t i=0;i<samples;++i) data[i]*=gain;
        return true;
    }
    if(encoding==ToneSampleEncoding::Signed16) {
        auto* data=static_cast<std::int16_t*>(buffer);
        for(std::size_t i=0;i<samples;++i)
            data[i]=static_cast<std::int16_t>(static_cast<float>(data[i])*gain);
        return true;
    }
    return false;
}
} // namespace axv
