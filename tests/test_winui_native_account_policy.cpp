#include "../src/frontend_shared/axv_saved_account_policy.h"
// Keep security assertions active with MSVC Release /DNDEBUG.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
using axv::SavedAccountStatus;
using axv::SavedSipFields;
int main(){
    SavedSipFields f{L"pbx.example.com",L"agent1",L"5060",L"udp",L""};
    auto check=axv::validate_saved_sip_account(f);
    assert(check.status==SavedAccountStatus::Ready);
    assert(check.account.port==5060 && !check.account.use_tls);
    f.transport=L"";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::MissingTransport);
    f.transport=L"unknown";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Invalid);
    f.transport=L"tls";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::MissingCa);
    f.ca_file=L"C:\\lab\\cert.pem";check=axv::validate_saved_sip_account(f);
    assert(check.status==SavedAccountStatus::Ready && check.account.use_tls);
    f.port=L"0";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Invalid);
    f.port=L"65000abc";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Invalid);
    f.port=L"65536";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Invalid);
    f.port=L"5060 ";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Invalid);
    f.port=L"5061";f.server=L"";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Invalid);
    f.server=L"pbx.example.com";f.user=L"agent with spaces";
    assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Invalid);
    for(unsigned i=1;i<=1000;++i){
        f.user=L"agent1";f.port=std::to_wstring(i);
        assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::Ready);
        f.transport=L"";assert(axv::validate_saved_sip_account(f).status==SavedAccountStatus::MissingTransport);
        f.transport=L"tls";
    }
    std::puts("WinUI3 saved SIP metadata: 1000 explicit TLS + 1000 invalid transport cases");
}
