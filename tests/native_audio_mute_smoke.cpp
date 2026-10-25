#include "../src/baresip_sdk_headers.h"
#include <cstring>
// Real SDK audio object, RTP disabled and no source/player started.
int main(){
 if(libre_init())return 1;
 const char* config="audio_source wasapi,default\naudio_player wasapi,default\n";
 if(conf_configure_buf(reinterpret_cast<const uint8_t*>(config),std::strlen(config)))return 2;
 struct sa local;sa_set_str(&local,"127.0.0.1",0);
 struct sdp_session* sdp=nullptr;
 if(sdp_session_alloc(&sdp,&local))return 3;
 struct list streams{},codecs{};
 struct stream_param stream{false,false,AF_INET,"axe-local-mute","local"};
 struct audio* audio=nullptr;
 const int result=audio_alloc(&audio,&streams,&stream,conf_config(),nullptr,sdp,nullptr,nullptr,nullptr,nullptr,20,&codecs,true,nullptr,nullptr,nullptr,nullptr);
 if(result){mem_deref(sdp);conf_close();libre_close();return 4;}
 bool valid=!audio_ismuted(audio);
 audio_mute(audio,true);valid=valid && audio_ismuted(audio);
 audio_mute(audio,true);valid=valid && audio_ismuted(audio);
 audio_mute(audio,false);valid=valid && !audio_ismuted(audio);
 mem_deref(audio);mem_deref(sdp);conf_close();libre_close();
 return valid?0:5;
}
