#pragma once
#include "../sip_account.h"
#include "../native_account_config.h"
#include <algorithm>
#include <string>

namespace axv {
// Non-secret account identifiers saved ONLY after an explicit remember-password
// choice in Win32. Missing transport is not treated as UDP.
struct SavedSipFields {
    std::wstring server,user,port,transport,ca_file;
};
enum class SavedAccountStatus {Ready,MissingTransport,MissingCa,Invalid};
struct SavedAccountResult {
    SavedAccountStatus status=SavedAccountStatus::Invalid;
    SipAccount account{};
    std::wstring ca_file;
};
inline SavedAccountResult validate_saved_sip_account(const SavedSipFields& f) {
    if(f.transport.empty())return {SavedAccountStatus::MissingTransport,{},{}};
    if(f.transport!=L"udp" && f.transport!=L"tls")
        return {SavedAccountStatus::Invalid,{},{}};
    if(f.port.empty() || f.port.size()>5 ||
       !std::all_of(f.port.begin(),f.port.end(),
                    [](wchar_t ch){return ch>=L'0' && ch<=L'9';}))
        return {SavedAccountStatus::Invalid,{},{}};
    unsigned long port=0;
    for(wchar_t ch:f.port)port=port*10u+static_cast<unsigned>(ch-L'0');
    if(port==0 || port>65535)return {SavedAccountStatus::Invalid,{},{}};
    const bool tls=f.transport==L"tls";
    if(tls && f.ca_file.empty())return {SavedAccountStatus::MissingCa,{},{}};
    if(f.server.size()>253 || f.user.size()>128 ||
       (tls && (f.ca_file.size()>240 || f.ca_file.front()==L'#' ||
                f.ca_file.find(L'"')!=std::wstring::npos)))
        return {SavedAccountStatus::Invalid,{},{}};
    SipAccount account{f.server,f.user,static_cast<unsigned short>(port),tls};
    if(validate_account(account)!=AccountError::None ||
       !native_account_config(account,tls?f.ca_file:L"").valid)
        return {SavedAccountStatus::Invalid,{},{}};
    return {SavedAccountStatus::Ready,account,tls?f.ca_file:L""};
}
} // namespace axv
