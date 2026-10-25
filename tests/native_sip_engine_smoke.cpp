#include "../src/baresip_sip_engine.h"
#include <chrono>
#include <thread>
#include <cstdlib>
int main(int argc,char**argv) {
    axv::BaresipSipEngine engine;
    if(engine.status()!=axv::SipStatus::Disconnected || engine.start_call(L"100")) return 1;
    if(argc==1) { engine.stop(); return 0; }
    axv::SipAccount account{L"127.0.0.1",L"1001",static_cast<unsigned short>(std::atoi(argv[1])),false};
    const bool success=std::string(argv[2])=="success";
    std::wstring ca;
    if(argc>3) {std::string path=argv[3];ca.assign(path.begin(),path.end());account.use_tls=true;}
    if(!engine.configure_authenticated(account,ca,success?"local-test-secret":"wrong-secret",1500)) return 2;
    if(!engine.register_account()) return 3;
    const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
    while(std::chrono::steady_clock::now()<until) {
        auto s=engine.status();
        if(s==axv::SipStatus::Registered || s==axv::SipStatus::Failed) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    const auto observed=engine.observation();
    if(success && (engine.status()!=axv::SipStatus::Registered ||
       !axv::sip_proof_is_current(observed,observed.session))) return 4;
    if(!success && engine.status()!=axv::SipStatus::Failed) return 5;
    engine.stop();
    if(engine.status()!=axv::SipStatus::Disconnected || engine.start_call(L"100") || engine.observation().session) return 6;
    // Restart creates a fresh callback epoch and destroys SDK state safely.
    if(!engine.configure_authenticated(account,ca,success?"local-test-secret":"wrong-secret",1500) || !engine.register_account()) return 7;
    std::this_thread::sleep_for(std::chrono::milliseconds(60)); engine.stop();
    return 0;
}
