#pragma once
#include "sip_account.h"
#include "sip_address.h"
#include "native_audio_routes.h"
#include "native_destination_result.h"
#include <string>
#include <utility>
namespace axv {
struct NativeAccountConfig { bool valid=false; std::string aor,config; };
inline std::string native_ascii(const std::wstring& value) {
    std::string out;
    for (wchar_t c:value) { if(c<32 || c>126) return {}; out+=static_cast<char>(c); }
    return out;
}
inline NativeAccountConfig native_account_config(const SipAccount& a,const std::wstring& ca,const NativeAudioRoute& audio={}) {
    if(!valid_native_audio_route(audio))return {};
    if(validate_account(a)!=AccountError::None || a.server.size()>253 || a.username.size()>128) return {};
    const auto host=native_ascii(a.server),user=native_ascii(a.username),trust=native_ascii(ca);
    if(host.empty() || user.empty()) return {};
    for(char c:user) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'||c=='+')) return {};
    if(a.use_tls && (trust.empty() || trust.size()>240 || trust.find('"')!=std::string::npos || trust.front()=='#')) return {};
    const std::string transport=a.use_tls?"tls":"udp";
    NativeAccountConfig out;
    out.aor="<sip:"+user+"@"+host+":"+std::to_string(a.port)+";transport="+transport+">;regint=0;dtmfmode=info;audio_codecs=PCMU/8000/1,PCMA/8000/1";
    const auto capture=audio.capture_id.empty()?std::string("default"):native_ascii(audio.capture_id);
    const auto playback=audio.playback_id.empty()?std::string("default"):native_ascii(audio.playback_id);
    out.config="poll_method select\nsip_transports "+transport+"\nsip_verify_server yes\naudio_player wasapi,"+playback+"\naudio_source wasapi,"+capture+"\naudio_alert wasapi,default\ncall_max_calls 1\n";
    // Upstream excludes loopback from automatic interface discovery. Windows
    // cannot send to 127.0.0.1 through a SIP socket bound to a LAN address.
    if(a.server==L"127.0.0.1") out.config+="net_interface 127.0.0.1\n";
    if(a.use_tls) out.config+="sip_cafile "+trust+"\n";
    out.valid=true; return out;
}
// The native engine enables exactly one SIP transport. Preserve explicit intent
// across GUI normalization and reject mismatched schemes before routing.
inline NativeDestinationResult native_call_destination_result(const std::wstring& typed,const SipAccount& a) {
    using Error=NativeDestinationError;
    if(typed.size()>512) return {Error::TooLong,{}};
    const auto parsed=validate_address(typed);
    if(!parsed) return {Error::InvalidAddress,{}};
    std::string uri=native_ascii(parsed.normalized);
    if(uri.empty()) return {Error::UnsupportedCharacters,{}};
    const bool secure=uri.compare(0,5,"sips:")==0;
    const bool plain=uri.compare(0,4,"sip:")==0;
    if((secure && !a.use_tls) || (plain && a.use_tls)) return {Error::TransportMismatch,{}};
    if(uri.find('@')==std::string::npos) {
        if(!native_account_config(a,a.use_tls?L"validated-ca":L"").valid) return {Error::InvalidAccount,{}};
        if(secure || plain) uri.erase(0,secure?5:4);
        uri=std::string(a.use_tls?"sips:":"sip:")+uri+"@"+native_ascii(a.server)+":"+std::to_string(a.port)+(a.use_tls?";transport=tls":";transport=udp");
    }
    if(uri.size()>512) return {Error::TooLong,{}};
    const auto start=uri.compare(0,5,"sips:")==0?5u:4u;
    const auto at=uri.find('@');
    if(at==std::string::npos || at<=start) return {Error::InvalidAddress,{}};
    // Keep native call users within the same supported grammar as REFER.
    for(std::size_t i=start;i<at;++i){
        const char c=uri[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'||c=='+'||c=='*'||c=='#')) return {Error::UnsupportedCharacters,{}};
    }
    return {Error::None,std::move(uri)};
}
inline std::string native_call_destination(const std::wstring& typed,const SipAccount& a) {
    return native_call_destination_result(typed,a).uri;
}
} // namespace axv
