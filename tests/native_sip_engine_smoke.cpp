#include "../src/baresip_sip_engine.h"
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstdio>
int main(int argc,char**argv) {
    axv::BaresipSipEngine engine;
    if(engine.status()!=axv::SipStatus::Disconnected || engine.start_call(L"100")) return 1;
    if(argc==1) { engine.stop(); return 0; }
    axv::SipAccount account{L"127.0.0.1",L"1001",static_cast<unsigned short>(std::atoi(argv[1])),false};
    const bool success=std::string(argv[2])=="success";
    std::wstring ca;
    if(argc>3) {std::string path=argv[3];ca.assign(path.begin(),path.end());account.use_tls=true;}
    if(!engine.configure_authenticated(account,ca,(std::string(argv[2])=="wrong-password")?"wrong-secret":"local-test-secret",1500)) return 2;
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
    std::fwprintf(stderr,L"%ls\n",engine.diagnostic().c_str());
    if(success && (engine.status()!=axv::SipStatus::Registered ||
       !axv::sip_proof_is_current(observed,observed.session))) return 4;
    if(!success && engine.status()!=axv::SipStatus::Failed) return 5;
    engine.stop();
    if(engine.status()!=axv::SipStatus::Disconnected || engine.start_call(L"100") || engine.observation().session) return 6;
    // Restart creates a fresh callback epoch and destroys SDK state safely.
    if(!engine.configure_authenticated(account,ca,(std::string(argv[2])=="wrong-password")?"wrong-secret":"local-test-secret",1500) || !engine.register_account()) return 7;
    std::this_thread::sleep_for(std::chrono::milliseconds(60)); engine.stop();
    return 0;
}
