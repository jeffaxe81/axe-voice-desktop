#pragma once
// Windows Credential Manager: current Windows user only. Never write SIP
// passwords to the registry, application log, or project configuration.
#include <windows.h>
#include <wincred.h>
#include <string>
#include <string_view>
#include <optional>
#include <limits>

namespace axv {
inline constexpr wchar_t kSipCredentialTarget[]=L"AXEVoice/Desktop/SIP/Default";

struct SavedSipCredential {
    std::wstring server,user,port,password;
};

inline std::wstring sip_credential_identity(
    std::wstring_view server,std::wstring_view user,std::wstring_view port) {
    if(server.empty() || user.empty() || server.find(L'\n')!=server.npos ||
       user.find(L'\n')!=user.npos || port.find(L'\n')!=port.npos ||
       server.size()>255 || user.size()>255 || port.size()>32)return {};
    std::wstring id(server);
    id+=L'\n';id+=user;id+=L'\n';id+=port;
    return id;
}
inline bool sip_credential_parse_identity(
        std::wstring_view input,SavedSipCredential& out) {
    const auto first=input.find(L'\n');
    if(first==input.npos)return false;
    const auto second=input.find(L'\n',first+1);
    if(second==input.npos || input.find(L'\n',second+1)!=input.npos)return false;
    const std::wstring server(input.substr(0,first));
    const std::wstring user(input.substr(first+1,second-first-1));
    const std::wstring port(input.substr(second+1));
    if(sip_credential_identity(server,user,port)!=input)return false;
    out.server=server;out.user=user;out.port=port;
    return true;
}
inline void securely_clear_sip_secret(std::wstring& secret){
    if(!secret.empty())SecureZeroMemory(secret.data(),secret.size()*sizeof(wchar_t));
    secret.clear();
}
inline std::optional<SavedSipCredential> read_saved_sip_credential(){
    PCREDENTIALW found=nullptr;
    if(!CredReadW(kSipCredentialTarget,CRED_TYPE_GENERIC,0,&found) || !found)
        return std::nullopt;
    std::optional<SavedSipCredential> output;
    if(found->UserName &&
       found->CredentialBlobSize>0 &&
       found->CredentialBlobSize<=CRED_MAX_CREDENTIAL_BLOB_SIZE &&
       found->CredentialBlobSize%sizeof(wchar_t)==0){
        SavedSipCredential parsed{};
        const std::wstring_view name(found->UserName);
        if(sip_credential_parse_identity(name,parsed)){
            const auto length=found->CredentialBlobSize/sizeof(wchar_t);
            parsed.password.assign(
                reinterpret_cast<const wchar_t*>(found->CredentialBlob),
                static_cast<std::size_t>(length));
            if(!parsed.password.empty() && parsed.password.find(L'\0')==parsed.password.npos)
                output=std::move(parsed);
            else securely_clear_sip_secret(parsed.password);
        }
    }
    CredFree(found);
    return output;
}
inline bool save_sip_credential(const SavedSipCredential& data){
    const auto identity=sip_credential_identity(data.server,data.user,data.port);
    if(identity.empty() || data.password.empty() ||
       data.password.size()>CRED_MAX_CREDENTIAL_BLOB_SIZE/sizeof(wchar_t) ||
       data.password.find(L'\0')!=data.password.npos)return false;
    CREDENTIALW credential{};
    credential.Type=CRED_TYPE_GENERIC;
    credential.TargetName=const_cast<LPWSTR>(kSipCredentialTarget);
    credential.UserName=const_cast<LPWSTR>(identity.c_str());
    credential.CredentialBlobSize=static_cast<DWORD>(data.password.size()*sizeof(wchar_t));
    credential.CredentialBlob=reinterpret_cast<LPBYTE>(
        const_cast<wchar_t*>(data.password.data()));
    credential.Persist=CRED_PERSIST_LOCAL_MACHINE;
    return CredWriteW(&credential,0)!=FALSE;
}
inline bool delete_saved_sip_credential(){
    if(CredDeleteW(kSipCredentialTarget,CRED_TYPE_GENERIC,0))return true;
    return GetLastError()==ERROR_NOT_FOUND;
}
inline bool saved_credential_matches(const SavedSipCredential& data,
 std::wstring_view server,std::wstring_view user,std::wstring_view port){
    return data.server==server && data.user==user && data.port==port;
}
}
