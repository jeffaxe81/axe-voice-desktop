#include "../src/baresip_sdk_headers.h"
#include "../src/native_account_config.h"
#include <cstring>
int main(){
 if(libre_init())return 1;
 axv::SipAccount account{L"127.0.0.1",L"1001",5060,false};
 auto profile=axv::native_account_config(account,L"",{L"capture-id",L"playback-id"});
 if(!profile.valid || conf_configure_buf(reinterpret_cast<const uint8_t*>(profile.config.data()),profile.config.size()))return 2;
 const auto& audio=conf_config()->audio;
 bool valid=!std::strcmp(audio.src_mod,"wasapi") && !std::strcmp(audio.src_dev,"capture-id") &&
            !std::strcmp(audio.play_mod,"wasapi") && !std::strcmp(audio.play_dev,"playback-id");
 profile=axv::native_account_config(account,L"");
 if(conf_configure_buf(reinterpret_cast<const uint8_t*>(profile.config.data()),profile.config.size()))return 3;
 valid=valid && !std::strcmp(audio.src_dev,"default") && !std::strcmp(audio.play_dev,"default");
 profile=axv::native_account_config(account,L"",{std::wstring(127,L'c'),std::wstring(127,L'p')});
 if(!profile.valid || conf_configure_buf(reinterpret_cast<const uint8_t*>(profile.config.data()),profile.config.size()))return 5;
 valid=valid && std::strlen(audio.src_dev)==127 && std::strlen(audio.play_dev)==127 && audio.src_dev[126]=='c' && audio.play_dev[126]=='p';
 conf_close();libre_close();
 return valid?0:4;
}
