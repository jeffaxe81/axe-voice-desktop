#include "../src/baresip_sdk_secret.h"
#include <cstring>
#include "../src/native_account_config.h"
int main(){
 if(libre_init())return 1;
 const char* config="sip_transports udp\n";
 if(conf_configure_buf(reinterpret_cast<const uint8_t*>(config),std::strlen(config)) || baresip_init(conf_config()))return 2;
 struct account* account=nullptr;
 const auto profile=axv::native_account_config({L"127.0.0.1",L"1001",5060,false},L"");
 // Codec modules are not loaded by this focused account/secret parser probe.
 const auto aor=profile.aor.substr(0,profile.aor.find(";audio_codecs="));
 if(!profile.valid || account_alloc(&account,aor.c_str()) || account_dtmfmode(account)!=DTMFMODE_SIP_INFO || account_set_auth_pass(account,"temporary-secret"))return 3;
 // SIP Digest holds mem_ref() of the same allocation in pinned4.12.
 const auto size=std::strlen(account_auth_pass(account));
 const char* held=static_cast<const char*>(mem_ref(const_cast<char*>(account_auth_pass(account))));
 axv::erase_sdk_account_password(account);
 bool zero=account_auth_pass(account)==nullptr;
 for(std::size_t i=0;i<size;++i)zero=zero && held[i]==0;
 mem_deref(const_cast<char*>(held));mem_deref(account);
 conf_close();baresip_close();libre_close();
 return zero?0:4;
}
