#include "../src/native_account_config.h"
#include <cstdlib>
#include <initializer_list>
#define CHECK(x) do { if (!(x)) std::abort(); } while (0)
int main() {
    axv::SipAccount account{L"pbx.example",L"1001",5060,false};
    auto cfg=axv::native_account_config(account,L"");
    CHECK(cfg.aor.find(";dtmfmode=info")!=std::string::npos);
    axv::SipAccount local{L"127.0.0.1",L"1001",5060,false};
    CHECK(axv::native_account_config(local,L"").config.find("net_interface 127.0.0.1")!=std::string::npos);
    CHECK(cfg.config.find("net_interface")==std::string::npos);
    CHECK(cfg.valid && cfg.aor=="<sip:1001@pbx.example:5060;transport=udp>;regint=0;dtmfmode=info;audio_codecs=PCMU/8000/1,PCMA/8000/1");
    // Codec choice must affect the actual native registration AOR/SDP policy.
    account.codec_mode=axv::NativeCodecMode::Pcma;
    auto manual=axv::native_account_config(account,L"");
    CHECK(manual.valid && manual.aor.find("audio_codecs=PCMA/8000/1")!=std::string::npos);
    CHECK(manual.aor.find("PCMU/8000/1")==std::string::npos);
    account.codec_mode=axv::NativeCodecMode::Pcmu;
    manual=axv::native_account_config(account,L"");
    CHECK(manual.valid && manual.aor.find("audio_codecs=PCMU/8000/1")!=std::string::npos);
    CHECK(manual.aor.find("PCMA/8000/1")==std::string::npos);
    account.codec_mode=static_cast<axv::NativeCodecMode>(99);
    CHECK(!axv::native_account_config(account,L"").valid);
    account.codec_mode=axv::NativeCodecMode::Automatic;
    CHECK(axv::native_account_config(account,L"").aor.find("PCMU/8000/1,PCMA/8000/1")!=std::string::npos);
    CHECK(axv::native_call_destination(L"101",account)=="sip:101@pbx.example:5060;transport=udp");
    CHECK(axv::native_call_destination(L"sip:200@elsewhere.example",account)=="sip:200@elsewhere.example");
    CHECK(axv::native_call_destination(L"100\r\nInjected: x",account).empty());
    // Customer-like UDP/5060 profile, deliberately using synthetic identity.
    // No password, actual SIP identity or remote registration is stored here.
    const axv::SipAccount udp_profile{L"sip.example.invalid",L"test-extension",5060,false};
    const auto udp_cfg=axv::native_account_config(udp_profile,L"");
    CHECK(udp_cfg.valid);
    CHECK(udp_cfg.config.find("sip_transports udp")!=std::string::npos);
    CHECK(udp_cfg.config.find("sip_transports tls")==std::string::npos);
    CHECK(udp_cfg.config.find("sip_cafile")==std::string::npos);
    CHECK(udp_cfg.aor.find(":5060;transport=udp")!=std::string::npos);
    CHECK(axv::native_call_destination(L"1002",udp_profile)==
          "sip:1002@sip.example.invalid:5060;transport=udp");
    CHECK(axv::native_call_destination(L"sips:1002@sip.example.invalid",udp_profile).empty());
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

    // The account's configured port is the REMOTE registrar destination.
    // It must not silently become a local listening port or a firewall
    // configuration. Real Contact/rport/NAT behavior still needs PBX testing.
    for(const unsigned short registrar_port:{5060,5061,5070,5080,65000}) {
        const axv::SipAccount remote{L"pbx.example",L"1234",registrar_port,false};
        const auto selected=axv::native_account_config(remote,L"");
        CHECK(selected.valid);
        CHECK(selected.aor.find(":"+std::to_string(registrar_port)+";transport=udp")!=std::string::npos);
        CHECK(selected.config.find("sip_transports udp\n")!=std::string::npos);
        CHECK(selected.config.find("sip_listen")==std::string::npos);
        CHECK(axv::native_call_destination(L"12345",remote)==
              "sip:12345@pbx.example:"+std::to_string(registrar_port)+";transport=udp");
    }
    // A default/invalid port must not produce an AOR or a callable profile.
    axv::SipAccount invalid{L"pbx.example",L"1234",0,false};
    CHECK(!axv::native_account_config(invalid,L"").valid);
    CHECK(axv::native_call_destination(L"12345",invalid).empty());
    for(const std::wstring& bad_host:std::initializer_list<std::wstring>{
        std::wstring{},L"bad host",L"bad\r\nname",L"@pbx.example",
        L"pbx.example;transport=tls",L"pbx.example\tother"
    }) {
        invalid.server=bad_host;invalid.port=5060;
        CHECK(!axv::native_account_config(invalid,L"").valid);
    }
    invalid.server=L"pbx.example";invalid.use_tls=true;invalid.port=5061;
    for(const std::wstring& bad_ca:std::initializer_list<std::wstring>{std::wstring{},L"#comment",L"bad\nfile",L"bad\rfile",L"bad\"quote"}) {
        CHECK(!axv::native_account_config(invalid,bad_ca).valid);
    }
    const auto tls_cfg=axv::native_account_config(invalid,L"C:/ca/trust.pem");
    CHECK(tls_cfg.valid);
    CHECK(tls_cfg.config.find("sip_transports tls\n")!=std::string::npos);
    CHECK(tls_cfg.config.find("sip_cafile C:/ca/trust.pem\n")!=std::string::npos);
    CHECK(tls_cfg.config.find("sip_listen")==std::string::npos);
    CHECK(tls_cfg.aor.find(":5061;transport=tls")!=std::string::npos);
    // Length and label-edge cases are public parser contracts, not DNS
    // resolution claims. Keep malformed hosts out of the native AOR.
    const std::wstring label63(63,L'a'),label64(64,L'b');
    const std::wstring host253=label63+L"."+label63+L"."+label63+L"."+std::wstring(61,L'c');
    const std::wstring host254=label63+L"."+label63+L"."+label63+L"."+std::wstring(62,L'c');
    CHECK(host253.size()==253 && host254.size()==254);
    CHECK(axv::valid_sip_host(label63));
    CHECK(!axv::valid_sip_host(label64));
    CHECK(axv::valid_sip_host(host253));
    CHECK(!axv::valid_sip_host(host254));
    invalid.use_tls=false;invalid.port=5060;
    invalid.server=host253;
    CHECK(axv::native_account_config(invalid,L"").valid);
    invalid.server=host254;
    CHECK(!axv::native_account_config(invalid,L"").valid);
    for(const std::wstring& malformed:std::initializer_list<std::wstring>{
        L".pbx.example",L"pbx.example.",L"pbx..example",
        L"-pbx.example",L"pbx-.example",L"pbx.example/extra",
        L"pbx_example",L"pbx.example:5060"
    }) {
        invalid.server=malformed;
        CHECK(!axv::native_account_config(invalid,L"").valid);
    }
    invalid.server=L"pbx.example";invalid.username=std::wstring(128,L'1');
    CHECK(axv::native_account_config(invalid,L"").valid);
    invalid.username.push_back(L'1');
    CHECK(!axv::native_account_config(invalid,L"").valid);
}


