#include "../src/baresip_sdk_secret.h"
#include <cstring>
int main(){
 if(libre_init())return 1;
 const char* config="sip_transports udp\n";
 if(conf_configure_buf(reinterpret_cast<const uint8_t*>(config),std::strlen(config)) || baresip_init(conf_config()))return 2;
 struct account* account=nullptr;
 if(account_alloc(&account,"<sip:1001@127.0.0.1>;regint=0") || account_set_auth_pass(account,"temporary-secret"))return 3;
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
