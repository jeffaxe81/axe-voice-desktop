#include "../src/destination_feedback.h"
#include "../src/account_form_feedback.h"
#include <iostream>
#include <string>
#include <string_view>

int main() {
    using namespace axv;
    int cases=0;
    for(int i=0;i<1000;++i) {
        const std::wstring number=std::to_wstring(1000+i);
        const auto valid=evaluate_destination(number);
        if(!valid.valid() ||
           std::wstring_view(valid.message).find(L"Ramal valido")==std::wstring_view::npos ||
           std::wstring_view(valid.message).find(L"registro SIP real")==std::wstring_view::npos)
            return 1;
        const auto invalid=evaluate_destination(number+L" ");
        if(invalid.valid() || invalid.kind!=DestinationFeedbackKind::Invalid ||
           std::wstring_view(invalid.message).find(L"espacos")==std::wstring_view::npos)
            return 2;
        ++cases;
    }
    const auto sip=evaluate_destination(L"ana@example.org");
    if(!sip.valid() ||
       std::wstring_view(sip.message).find(L"Endereco SIP valido")==std::wstring_view::npos)
        return 3;
    const auto http=evaluate_destination(L"https://example.org");
    if(http.valid() || http.error!=AddressError::InvalidScheme) return 4;
    const auto account=evaluate_account_form(L"pbx.example.org",L"1001",L"5060");
    if(!account.valid() || account.port!=5060 ||
       std::wstring_view(account.message).find(L"Registro SIP nao realizado")==std::wstring_view::npos)
        return 5;
    const auto bad_port=evaluate_account_form(L"pbx.example.org",L"1001",L"65536");
    if(bad_port.valid() ||
       std::wstring_view(bad_port.message).find(L"intervalo")==std::wstring_view::npos)
        return 6;
    const auto missing=evaluate_account_form(L"",L"1001",L"5060");
    if(missing.valid() || missing.kind!=AccountFeedbackKind::Incomplete) return 7;
    if(cases!=1000) return 8;
    std::cout<<"1000 destination feedback matrix pairs verified; no SIP registration.\n";
    return 0;
}
