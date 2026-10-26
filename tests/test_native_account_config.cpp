#include "../src/native_account_config.h"
#include <cstdlib>
#define CHECK(x) do { if (!(x)) std::abort(); } while (0)
int main() {
    axv::SipAccount account{L"pbx.example",L"1001",5060,false};
    auto cfg=axv::native_account_config(account,L"");
    CHECK(cfg.aor.find(";dtmfmode=info")!=std::string::npos);
    axv::SipAccount local{L"127.0.0.1",L"1001",5060,false};
    CHECK(axv::native_account_config(local,L"").config.find("net_interface 127.0.0.1")!=std::string::npos);
    CHECK(cfg.config.find("net_interface")==std::string::npos);
    CHECK(cfg.valid && cfg.aor=="<sip:1001@pbx.example:5060;transport=udp>;regint=0;dtmfmode=info;audio_codecs=PCMU/8000/1,PCMA/8000/1");
    CHECK(axv::native_call_destination(L"101",account)=="sip:101@pbx.example:5060;transport=udp");
    CHECK(axv::native_call_destination(L"sip:200@elsewhere.example",account)=="sip:200@elsewhere.example");
    CHECK(axv::native_call_destination(L"100\r\nInjected: x",account).empty());
    account.username=L"100;auth_pass=bad"; CHECK(!axv::native_account_config(account,L"").valid);
    account.username=L"1001"; account.use_tls=true; account.port=5061;
    CHECK(!axv::native_account_config(account,L"").valid);
    CHECK(!axv::native_account_config(account,L"x\nignore").valid);
    cfg=axv::native_account_config(account,L"C:/certificates/pbx.pem");
    CHECK(cfg.valid && cfg.config.find("sip_verify_server yes")!=std::string::npos);
    CHECK(cfg.config.find("sip_cafile C:/certificates/pbx.pem")!=std::string::npos);
    CHECK(cfg.aor.find("sips:")==std::string::npos); // SIP URI over explicit TLS transport.
    CHECK(cfg.aor.find("transport=tls")!=std::string::npos);
    CHECK(axv::native_call_destination(L"sip:2@pbx.example",account).empty()); // no TLS downgrade
    CHECK(axv::native_call_destination(L"2",account)=="sips:2@pbx.example:5061;transport=tls");
    CHECK(axv::native_call_destination(L"sips:2@pbx.example",account)=="sips:2@pbx.example");
    account.server=L"bad\nserver"; CHECK(!axv::native_account_config(account,L"ca.pem").valid);
}
