#pragma once
#include "sip_account.h"
#include "sip_address.h"
#include <string>
namespace axv {
struct NativeAccountConfig { bool valid=false; std::string aor,config; };
inline std::string native_ascii(const std::wstring& value) {
    std::string out;
    for (wchar_t c:value) { if(c<32 || c>126) return {}; out+=static_cast<char>(c); }
    return out;
}
inline NativeAccountConfig native_account_config(const SipAccount& a,const std::wstring& ca) {
    if(validate_account(a)!=AccountError::None || a.server.size()>253 || a.username.size()>128) return {};
    const auto host=native_ascii(a.server),user=native_ascii(a.username),trust=native_ascii(ca);
    if(host.empty() || user.empty()) return {};
    for(char c:user) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'||c=='+')) return {};
    if(a.use_tls && (trust.empty() || trust.size()>240 || trust.find('"')!=std::string::npos || trust.front()=='#')) return {};
    const std::string transport=a.use_tls?"tls":"udp";
    NativeAccountConfig out;
    out.aor="<sip:"+user+"@"+host+":"+std::to_string(a.port)+";transport="+transport+">;regint=0;audio_codecs=PCMU/8000/1,PCMA/8000/1";
    out.config="poll_method select\nsip_transports "+transport+"\nsip_verify_server yes\naudio_player wasapi,default\naudio_source wasapi,default\naudio_alert wasapi,default\ncall_max_calls 1\n";
    if(a.use_tls) out.config+="sip_cafile "+trust+"\n";
    out.valid=true; return out;
}
inline std::string native_call_destination(const std::wstring& typed,const SipAccount& a) {
    const auto parsed=validate_address(typed);
    if(!parsed) return {};
    std::string uri=native_ascii(parsed.normalized);
    if(uri.empty()) return {};
    if(uri.find('@')==std::string::npos) {
        if(!native_account_config(a,a.use_tls?L"validated-ca":L"").valid) return {};
        return std::string(a.use_tls?"sips:":"sip:")+uri+"@"+native_ascii(a.server)+":"+std::to_string(a.port)+(a.use_tls?";transport=tls":";transport=udp");
    }
    if(a.use_tls && uri.compare(0,5,"sips:")!=0) return {};
    return uri;
}
} // namespace axv
