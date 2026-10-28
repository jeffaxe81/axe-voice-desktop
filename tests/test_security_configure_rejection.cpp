// Actual SDK validation-rejection regression, with always-on Release checks.
// Synthetic WASAPI rejects before UA creation: stale register attempts send no SIP.
#include "../src/baresip_sdk_headers.h"
#include "../src/baresip_sip_engine.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

namespace {
std::atomic<unsigned> audio_attempts{0};
int module_ok(){return 0;}
int module_reject(){++audio_attempts;return 95;}
int module_close(){return 0;}
}
extern "C" {
extern const struct mod_export exports_g711={"synthetic_g711","codec",module_ok,module_close};
extern const struct mod_export exports_wasapi={"synthetic_wasapi","audio",module_reject,module_close};
}
namespace axv {AudioInventory inspect_windows_audio_devices(){return {};}}

int main(){
    unsigned failures=0;
    for(unsigned which=0;which<9;++which){
        axv::BaresipSipEngine engine;
        const axv::SipAccount first{L"127.0.0.1",L"1001",5060,true};
        if(!engine.configure_authenticated(first,L"synthetic-ca.pem","synthetic-A-only",1000))return 2;
        axv::SipAccount next{L"127.0.0.1",L"1002",5060,false};
        std::string password="synthetic-B-only";
        unsigned timeout=1000;
        axv::NativeAudioRoute audio;
        switch(which){
        case 0:next.server.clear();break;
        case 1:password.clear();break;
        case 2:password.assign(1025,'x');break;
        case 3:password=std::string("synthetic\0B",11);break;
        case 4:timeout=99;break;
        case 5:timeout=60001;break;
        case 6:next.use_tls=true;break; // Missing required CA.
        case 7:audio.capture_id=L"bad\nroute";break;
        case 8:break; // Unauthenticated generic configure must also revoke pending intent.
        }
        if(which==8?engine.configure(next):engine.configure_authenticated(next,L"",std::move(password),timeout,audio))return 3;
        const auto state=engine.security_status();
        const bool accepted=engine.register_account();
        if(accepted){
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            while(engine.status()!=axv::SipStatus::Failed && std::chrono::steady_clock::now()<deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if(engine.status()!=axv::SipStatus::Failed || engine.diagnostic().find(L"wasapi")==std::wstring::npos)return 4;
        }
        engine.stop();
        if(state.policy_selected || state.tls_required || state.running_process || state.verified_registration || accepted){
            std::fprintf(stderr,"inactive rejection case=%u retained_policy=%d retained_tls=%d stale_register_accepted=%d\n",which,state.policy_selected,state.tls_required,accepted);
            ++failures;
        }
    }
    if(failures){
        std::fprintf(stderr,"%u validation rejections retained stale intent; synthetic audio stops=%u\n",failures,audio_attempts.load());
        return 1;
    }
    return 0;
}
