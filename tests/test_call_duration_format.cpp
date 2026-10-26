#include "../src/call_duration_format.h"
#include <cstdio>
#include <limits>
#include <string>

namespace {
int failures = 0;
void expect(std::uint64_t milliseconds, const wchar_t* expected, const char* name) {
    const auto actual = axv::format_call_duration(milliseconds);
    if (actual != expected) {
        std::fprintf(stderr, "%s: unexpected duration format\n", name);
        ++failures;
    }
}
void check_maximum_components() {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    const auto actual = axv::format_call_duration(maximum);
    const auto first = actual.find(L':');
    const auto second = first == std::wstring::npos ? first : actual.find(L':', first + 1);
    if (first == std::wstring::npos || second == std::wstring::npos ||
        second != first + 3 || actual.size() != second + 3 ||
        actual.find_first_not_of(L"0123456789:") != std::wstring::npos) {
        std::fprintf(stderr, "maximum: invalid HH:MM:SS structure\n");
        ++failures;
        return;
    }
    const auto hours = std::stoull(actual.substr(0, first));
    const auto minutes = std::stoull(actual.substr(first + 1, 2));
    const auto seconds = std::stoull(actual.substr(second + 1, 2));
    // Divide before recombining: rounding up or overflowing milliseconds fails.
    const auto whole_seconds = maximum / 1000;
    if (hours != maximum / 3600000 || minutes != (maximum / 60000) % 60 ||
        seconds != whole_seconds % 60 || minutes >= 60 || seconds >= 60 ||
        hours * 3600 + minutes * 60 + seconds != whole_seconds ||
        axv::format_call_duration(maximum - maximum % 1000) != actual) {
        std::fprintf(stderr, "maximum: components must represent truncated seconds\n");
        ++failures;
    }
}
}

int main() {
    expect(0, L"00:00:00", "zero");
    expect(999, L"00:00:00", "subsecond truncation");
    expect(1000, L"00:00:01", "one second");
    expect(59999, L"00:00:59", "before minute boundary");
    expect(60000, L"00:01:00", "minute boundary");
    expect(3599999, L"00:59:59", "before hour boundary");
    expect(3600000, L"01:00:00", "hour boundary");
    expect(356400000, L"99:00:00", "two digit hours");
    expect(359999999, L"99:59:59", "before three digit hours");
    expect(360000000, L"100:00:00", "three digit hours");
    expect(444645999, L"123:30:45", "large hours retain minutes and seconds");
    check_maximum_components();
    return failures == 0 ? 0 : 1;
}
