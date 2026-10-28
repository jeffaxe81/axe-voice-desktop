// Actual SDK with test-only calls redirected in separately compiled objects.
// Hook mode throws before UA allocation; loop mode suppresses REGISTER and exits
// immediately without executing the native event loop. Mode3 holds the owner
// in a test-only loop to verify active reconfiguration isolation. No SIP traffic.
#include "../src/baresip_sdk_headers.h"
#include "../src/baresip_sip_engine.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <thread>
namespace {
int mode=1;
std::atomic<unsigned> entered{0};
std::atomic<bool> release_loop{false};
int module_ok(){return 0;}
}
extern "C" {
extern const struct mod_export exports_g711={"synthetic_g711","codec",module_ok,module_ok};
extern const struct mod_export exports_wasapi={"synthetic_wasapi","audio",module_ok,module_ok};
int axv_fixture_owner_mod_add(struct mod** out,const struct mod_export* value){
 ++entered;
 if(mode==1)throw std::bad_alloc();
 return mod_add(out,value);
}
int axv_fixture_owner_ua_register(struct ua*){return 0;}
int axv_fixture_owner_re_main(re_signal_h*){
 if(mode==3){while(!release_loop.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));return 0;}
 return 95;
}
}
namespace axv {AudioInventory inspect_windows_audio_devices(){return {};}}
int main(int argc,char**argv){
 mode=argc>1?std::atoi(argv[1]):1;
 if(mode<1 || mode>3)return 2;
 if(mode==3){
  axv::BaresipSipEngine engine;
  if(!engine.configure_authenticated({L"127.0.0.1",L"1001",5060,false},L"","synthetic-A",10000))return 3;
  const bool launched=engine.register_account();
  const auto before=engine.security_status();
  const axv::SipAccount other{L"127.0.0.1",L"1002",5061,true};
  const bool changed=engine.configure_authenticated(other,L"synthetic-ca.pem","synthetic-B",10000);
  const bool generic_changed=engine.configure(other);
  const auto after=engine.security_status();
  release_loop=true;engine.stop();
  if(!launched || changed || generic_changed || !before.policy_selected || before.tls_required ||
     !before.running_process || before.verified_registration || before.policy_selected!=after.policy_selected ||
     before.tls_required!=after.tls_required || before.running_process!=after.running_process ||
     before.verified_registration!=after.verified_registration)return 9;
  return 0;
 }
 for(unsigned attempt=1;attempt<=2;++attempt){
  axv::BaresipSipEngine engine;
  if(!engine.configure_authenticated({L"127.0.0.1",L"1001",5060,false},L"","synthetic-only",1000))return 3;
  const bool launched=engine.register_account();
  if(mode==1 && launched){engine.stop();return 4;}
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(engine.observation().process==axv::RuntimeProcess::Running && std::chrono::steady_clock::now()<deadline)
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
  const auto current=engine.observation();
  if(current.process!=axv::RuntimeProcess::Failed || current.sip!=axv::SipStatus::Failed){
   std::fputs("native execution fault was presented as normal stop\n",stderr);engine.stop();return 5;
  }
  const auto diagnostic=engine.diagnostic();
  const auto expected=mode==1?L"etapa do motor nativo":L"ciclo do motor nativo";
  if(diagnostic.find(expected)==std::wstring::npos){
   std::fputs("native fault stage is missing\n",stderr);engine.stop();return 6;
  }
  engine.stop();
  if(engine.status()!=axv::SipStatus::Disconnected || engine.diagnostic().find(expected)!=std::wstring::npos)return 7;
 }
 const auto expected_entries=mode==1?2u:4u;
 if(entered.load()!=expected_entries){std::fputs("partial startup retained process ownership\n",stderr);return 8;}
 return 0;
}
