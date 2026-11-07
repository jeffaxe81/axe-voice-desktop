#pragma once
#include <string_view>

namespace axv {
// Only the codecs loaded by the current native G.711 module are selectable.
// Automatic means both formats are offered for normal SDP negotiation.
enum class NativeCodecMode { Automatic = 0, Pcma = 1, Pcmu = 2 };

inline constexpr bool valid_native_codec_mode(NativeCodecMode mode) noexcept {
    return mode == NativeCodecMode::Automatic ||
           mode == NativeCodecMode::Pcma ||
           mode == NativeCodecMode::Pcmu;
}

inline constexpr std::string_view native_audio_codec_list(NativeCodecMode mode) noexcept {
    switch (mode) {
    case NativeCodecMode::Automatic: return "PCMU/8000/1,PCMA/8000/1";
    case NativeCodecMode::Pcma: return "PCMA/8000/1";
    case NativeCodecMode::Pcmu: return "PCMU/8000/1";
    }
    return {}; // Invalid values are rejected, never silently widened.
}
} // namespace axv
