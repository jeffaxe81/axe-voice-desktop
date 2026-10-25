#include "../src/destination_feedback.h"
#include <string>
#include <vector>
#include <utility>

int main() {
    using axv::DestinationFeedbackKind;
    const std::vector<std::pair<const wchar_t*, DestinationFeedbackKind>> cases = {
        {L"", DestinationFeedbackKind::Empty},
        {L"1001", DestinationFeedbackKind::Valid},
        {L"*21#", DestinationFeedbackKind::Valid},
        {L"+5547999999999", DestinationFeedbackKind::Valid},
        {L"sip:ana@example.org", DestinationFeedbackKind::Valid},
        {L"sips:ana@example.org:5061", DestinationFeedbackKind::Valid},
        {L"sip:", DestinationFeedbackKind::Invalid},
        {L"100 1", DestinationFeedbackKind::Invalid},
        {L"https://example.org", DestinationFeedbackKind::Invalid},
        {L"ana@@example.org", DestinationFeedbackKind::Invalid},
        {L"sip:ana@example.org:65536", DestinationFeedbackKind::Invalid}
    };
    for (const auto& item : cases) {
        const auto feedback = axv::evaluate_destination(item.first);
        if (feedback.kind != item.second || !feedback.message ||
            feedback.valid() != (item.second == DestinationFeedbackKind::Valid))
            return 1;
    }
    if (axv::evaluate_destination(L"").error != axv::AddressError::Empty) return 2;
    if (axv::evaluate_destination(L"10 01").error != axv::AddressError::Whitespace) return 3;
    if (axv::evaluate_destination(L"sip:ana@example.org:0").error != axv::AddressError::InvalidPort) return 4;
    if (axv::evaluate_destination(L"https://x").error != axv::AddressError::InvalidScheme) return 5;
    if (std::wstring(axv::evaluate_destination(L"1001").message).find(L"registro SIP real") == std::wstring::npos) return 6;
    return 0;
}
