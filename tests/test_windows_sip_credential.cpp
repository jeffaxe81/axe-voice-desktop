#include "windows_sip_credential.h"
#include <string>
#include <string_view>
int main(){
    axv::SavedSipCredential parsed;
    const auto valid=axv::sip_credential_identity(
        L"pbx.example.internal",L"1001",L"5061");
    if(valid.empty() || !axv::sip_credential_parse_identity(valid,parsed))return 1;
    if(parsed.server!=L"pbx.example.internal" || parsed.user!=L"1001" ||
       parsed.port!=L"5061")return 2;
    if(axv::sip_credential_parse_identity(L"broken",parsed) ||
       axv::sip_credential_parse_identity(L"host\nname\n5060\nexcess",parsed) ||
       axv::sip_credential_parse_identity(L"\nname\n5060",parsed))return 3;
    if(!axv::sip_credential_identity(L"server",L"1002\ninjection",L"5060").empty() ||
       !axv::sip_credential_identity(L"",L"1002",L"5060").empty())return 4;
    if(!axv::saved_credential_matches(parsed,L"pbx.example.internal",L"1001",L"5061"))
        return 5;
    if(axv::saved_credential_matches(parsed,L"other.internal",L"1001",L"5061") ||
       axv::saved_credential_matches(parsed,L"pbx.example.internal",L"1002",L"5061"))
        return 6;
    std::wstring secret=L"TestOnlyPass";
    axv::securely_clear_sip_secret(secret);
    if(!secret.empty())return 7;
    // Deliberately no CredWrite/CredRead: tests must not modify the user's vault.
    return 0;
}
