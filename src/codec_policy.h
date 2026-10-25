#pragma once
#include <array>
#include <string_view>
namespace axv {
enum class Codec { Opus, G722, PCMA, PCMU, G729 };
struct CodecCapability { Codec codec; std::string_view name; bool enabled_by_default; bool requires_external_validation; };
inline constexpr std::array<CodecCapability, 5> default_codec_capabilities{{
    {Codec::Opus, "opus", true, false},
    {Codec::G722, "G722", true, false},
    {Codec::PCMA, "PCMA", true, false},
    {Codec::PCMU, "PCMU", true, false},
    {Codec::G729, "G729", false, true}
}};
inline bool can_enable_codec(Codec codec, bool external_support_validated) {
    for (const auto& c : default_codec_capabilities) {
        if (c.codec == codec) return !c.requires_external_validation || external_support_validated;
    }
    return false;
}
}
