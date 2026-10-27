#include "../src/baresip_sip_engine.h"
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstdio>
int main(int argc,char**argv) {
    axv::BaresipSipEngine engine;
    if(engine.status()!=axv::SipStatus::Disconnected || engine.start_call(L"100") || engine.set_muted(true,{}) || engine.set_held(true,{}) || engine.transfer_call(L"1002",{}) || engine.send_dtmf(L'5',{}) || engine.answer_call({}) || engine.hangup_call({}) || engine.media_state().available) return 1;
    if(argc==1) {
        const auto initial_security=engine.security_status();
        if(initial_security.policy_selected || initial_security.tls_required || initial_security.running_process || initial_security.verified_registration)return 11;
        axv::SipAccount account{L"127.0.0.1",L"1001",5060,false};
        const axv::NativeAudioRoute route{L"capture-id",L"playback-id"};
        if(!engine.configure_authenticated(account,L"","fixture-only",1500,route))return 8;
        const auto applied=engine.selected_audio_route();
        if(applied.capture_id!=route.capture_id || applied.playback_id!=route.playback_id)return 9;
        account.use_tls=true; // Editing caller/UI input must not change configured policy.
        const auto udp_policy=engine.security_status();
        if(!udp_policy.policy_selected || udp_policy.tls_required || udp_policy.running_process || udp_policy.verified_registration)return 12;
        engine.stop();
        if(!engine.selected_audio_route().capture_id.empty())return 10;
        const auto stopped=engine.security_status();
        if(stopped.policy_selected || stopped.tls_required || stopped.running_process || stopped.verified_registration)return 13;
        if(!engine.configure_authenticated(account,L"synthetic-ca.pem","fixture-only",1500))return 14;
        const auto tls_policy=engine.security_status();
        if(!tls_policy.policy_selected || !tls_policy.tls_required || tls_policy.running_process || tls_policy.verified_registration)return 15;
        account.use_tls=false;
        if(!engine.security_status().tls_required)return 16;
        engine.stop();
        return 0;
    }
    axv::SipAccount account{L"127.0.0.1",L"1001",static_cast<unsigned short>(std::atoi(argv[1])),false};
    const std::string mode=argv[2];
    const bool missing_audio=mode=="missing-audio";
    const bool success=mode=="success" || missing_audio;
    const axv::NativeAudioRoute route=missing_audio?axv::NativeAudioRoute{L"missing-capture-device",L"missing-playback-device"}:axv::NativeAudioRoute{};
    std::wstring ca;
    if(argc>3) {std::string path=argv[3];ca.assign(path.begin(),path.end());account.use_tls=true;}
    if(!engine.configure_authenticated(account,ca,(std::string(argv[2])=="wrong-password")?"wrong-secret":"local-test-secret",1500,route)) return 2;
    if(!engine.register_account()) return 3;
    const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
    while(std::chrono::steady_clock::now()<until) {
        auto s=engine.status();
        if(s==axv::SipStatus::Registered || s==axv::SipStatus::Failed) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    if(success) std::this_thread::sleep_for(std::chrono::milliseconds(3500)); // include registrar refresh
    const auto observed=engine.observation();
    std::fprintf(stderr,"native status=%d process=%d evidence=%d session=%llu\n",
        static_cast<int>(observed.sip),static_cast<int>(observed.process),static_cast<int>(observed.evidence),static_cast<unsigned long long>(observed.session));
    const auto text=engine.diagnostic();
    const std::string diagnostic(text.begin(),text.end());
    std::fprintf(stderr,"%s\n",diagnostic.c_str());
    if(success && (engine.status()!=axv::SipStatus::Registered ||
       !axv::sip_proof_is_current(observed,observed.session))) return 4;
    if(!success && engine.status()!=axv::SipStatus::Failed) return 5;
    if(engine.set_muted(true,{}) || engine.set_held(true,{}) || engine.transfer_call(L"1002",{}) || engine.send_dtmf(L'5',{}) || engine.answer_call({}) || engine.hangup_call({}) || engine.media_state().available)return 13;
    if(missing_audio){
        if(!engine.start_call(L"1002"))return 11;
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
        bool denied=false;
        while(std::chrono::steady_clock::now()<deadline){
            const auto diagnostic=engine.diagnostic();
            denied=diagnostic==axv::native_audio_route_feedback(axv::NativeAudioRouteStatus::MissingCapture) ||
                   diagnostic==axv::native_audio_route_feedback(axv::NativeAudioRouteStatus::InventoryFailed);
            if(denied)break;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        if(!denied || engine.status()!=axv::SipStatus::Registered || engine.observation().call_id)return 12;
    }
    engine.stop();
    if(engine.status()!=axv::SipStatus::Disconnected || engine.start_call(L"100") || engine.observation().session) return 6;
    // Restart creates a fresh callback epoch and destroys SDK state safely.
    if(!engine.configure_authenticated(account,ca,(std::string(argv[2])=="wrong-password")?"wrong-secret":"local-test-secret",1500) || !engine.register_account()) return 7;
    std::this_thread::sleep_for(std::chrono::milliseconds(60)); engine.stop();
    return 0;
}
