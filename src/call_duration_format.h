#pragma once
#include <cstdint>
#include <string>

namespace axv {
inline std::wstring format_call_duration(std::uint64_t elapsed_ms) {
    const auto total_seconds = elapsed_ms / 1000;
    const auto hours = total_seconds / 3600;
    const auto minutes = (total_seconds / 60) % 60;
    const auto seconds = total_seconds % 60;
    auto result = std::to_wstring(hours);
    if (result.size() < 2) result.insert(0, 1, L'0');
    result += L':';
    if (minutes < 10) result += L'0';
    result += std::to_wstring(minutes);
    result += L':';
    if (seconds < 10) result += L'0';
    result += std::to_wstring(seconds);
    return result;
}
}
