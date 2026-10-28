// Actual SDK configure allocation-failure regression. No SDK startup or network.
// Always-on checks, including MSVC Release. Supply normal native engine sources
// and SDK libraries; synthetic module exports below avoid platform audio linkage.
#include "../src/baresip_sdk_headers.h"
#include "../src/baresip_sip_engine.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

namespace {
constexpr std::size_t secret_size=997;
char synthetic[secret_size+1];
void* tracked[32]{};
bool overflow=false;
bool inject=false;
bool injected=false;
int remaining=1;
unsigned raw_frees=0;
int module_ok(){return 0;}
std::size_t retained(){
    std::size_t n=0;
    for(auto p:tracked)if(p && !std::memcmp(p,synthetic,sizeof(synthetic)))++n;
    return n;
}
}
void* operator new(std::size_t n){
    if(inject && --remaining==0){inject=false;injected=true;throw std::bad_alloc();}
    void* p=std::malloc(n?n:1);
    if(!p)throw std::bad_alloc();
    if(n>=sizeof(synthetic)){
        bool found=false;
        for(auto& slot:tracked)if(!slot){slot=p;found=true;break;}
        if(!found)overflow=true;
    }
    return p;
}
void operator delete(void* p)noexcept{
    for(auto& slot:tracked)if(slot==p){
        if(p && !std::memcmp(p,synthetic,sizeof(synthetic)))++raw_frees;
        slot=nullptr;
    }
    std::free(p);
}
void operator delete(void* p,std::size_t)noexcept{::operator delete(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t)noexcept{::operator delete(p);}
extern "C" {
extern const struct mod_export exports_g711={"synthetic_g711","codec",module_ok,module_ok};
extern const struct mod_export exports_wasapi={"synthetic_wasapi","audio",module_ok,module_ok};
}
namespace axv {AudioInventory inspect_windows_audio_devices(){return {};}}
int main(int argc,char** argv){
    remaining=argc>1?std::atoi(argv[1]):1;
    if(remaining<1 || remaining>16)return 5;
    for(std::size_t i=0;i<secret_size;++i)synthetic[i]=static_cast<char>('!'+((i*31+7)%90));
    synthetic[secret_size]='\0';
    axv::BaresipSipEngine engine;
    const axv::SipAccount old_account{L"127.0.0.1",L"1001",5060,false};
    const axv::SipAccount replacement{L"127.0.0.1",L"1002",5060,false};
    const bool replacement_case=argc<3 || std::strcmp(argv[2],"initial")!=0;
    if(replacement_case && !engine.configure_authenticated(old_account,L"",std::string(synthetic),1000))return 2;
    std::string incoming(synthetic);
    if(overflow || retained()!=(replacement_case?2u:1u))return 3;
    bool threw=false,accepted=false;
    inject=true;
    try{accepted=engine.configure_authenticated(replacement,L"",std::move(incoming),1000);}
    catch(const std::bad_alloc&){threw=true;}
    inject=false;
    const auto pending=retained();
    const auto unsanitized=raw_frees;
    const auto state=engine.security_status();
    engine.stop();
    if(overflow)return 4;
    if(!injected){
        if(!accepted || threw || pending!=1 || unsanitized || retained())return 6;
        return 0;
    }
    if(state.policy_selected || state.tls_required || state.running_process || state.verified_registration)return 7;
    if(threw || accepted || pending || unsanitized){
        std::fprintf(stderr,"configure allocation rejection: escaped=%d accepted=%d pending_secret=%zu raw_secret_frees=%u\n",threw,accepted,pending,unsanitized);
        return 1;
    }
    return 0;
}
