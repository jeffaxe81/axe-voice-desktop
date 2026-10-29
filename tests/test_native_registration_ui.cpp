// Host-executable regression of the actual UI registration helper. All credentials synthetic.
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <new>
#include <string>
#include <cstdio>
#include <algorithm>
#include "registration_ui_fakes/windows.h"

namespace {
constexpr char credential[]="synthetic-credential-for-ui-allocation-regression";
bool armed=false;
long fail_after=-1;
unsigned unwiped_frees=0,wipes=0,wide_secret_wipes=0,injected_failures=0;
struct alignas(std::max_align_t) Allocation {std::size_t size;bool tracked;};
}
void* operator new(std::size_t n) {
    if(armed && fail_after>=0 && fail_after--==0){++injected_failures;throw std::bad_alloc();}
    auto* p=static_cast<Allocation*>(std::malloc(sizeof(Allocation)+n));
    if(!p)throw std::bad_alloc();
    p->size=n;p->tracked=armed;return p+1;
}
void operator delete(void* ptr) noexcept {
    if(!ptr)return;
    auto* p=static_cast<Allocation*>(ptr)-1;
    if(p->tracked){
        const char* bytes=static_cast<const char*>(ptr);
        for(std::size_t i=0;i+sizeof(credential)-1<=p->size;++i)
            if(std::memcmp(bytes+i,credential,sizeof(credential)-1)==0){++unwiped_frees;break;}
    }
    std::free(p);
}
void operator delete(void* p,std::size_t) noexcept {::operator delete(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete[](void* p,std::size_t) noexcept {::operator delete(p);}

struct RegistrationFakeControl {std::wstring text; bool password=false; unsigned reads=0;};
namespace { bool conversion_fail=false,conversion_fail_second=false; unsigned password_reads=0; }
int GetWindowTextW(HWND h,wchar_t* out,int n){
    const auto count=std::min(h->text.size(),static_cast<std::size_t>(n-1));
    std::wmemcpy(out,h->text.data(),count);out[count]=0;
    ++h->reads;if(h->password)++password_reads;return static_cast<int>(count);
}
int SetWindowTextW(HWND h,const wchar_t* value){h->text=value;return 1;}
LRESULT SendMessageW(HWND,UINT,WPARAM,LPARAM){return BST_CHECKED;}
int WideCharToMultiByte(UINT,DWORD,const wchar_t* in,int,char* out,int size,const char*,int*){
    if(conversion_fail || (conversion_fail_second && out))return 0;
    const int required=static_cast<int>(std::wcslen(in)+1);
    if(!out)return required;
    if(size<required)return 0;
    for(int i=0;i<required;++i)out[i]=static_cast<char>(in[i]);
    return required;
}
void* SecureZeroMemory(void* data,std::size_t n){
    constexpr wchar_t wide_credential[]=L"synthetic-credential-for-ui-allocation-regression";
    if(n>=sizeof(wide_credential) && std::memcmp(data,wide_credential,sizeof(wide_credential))==0)++wide_secret_wipes;
    volatile unsigned char* p=static_cast<volatile unsigned char*>(data);
    for(std::size_t i=0;i<n;++i)p[i]=0;
    ++wipes;return data;
}
#include "../src/native_registration_ui.h"

namespace {
struct Engine {
    bool configure_ok=true,register_ok=true,throw_configure=false,throw_register=false;
    unsigned configurations=0,registrations=0;
    unsigned short configured_port=0;
    bool configure_authenticated(const axv::SipAccount& account,const std::wstring&,std::string password,unsigned,const axv::NativeAudioRoute&){
        struct Guard{std::string& value;~Guard(){SecureZeroMemory(value.data(),value.size());}} guard{password};
        ++configurations;
        configured_port=account.port;
        if(password!=credential)throw 99;
        if(throw_configure)throw std::bad_alloc();
        return configure_ok;
    }
    bool register_account(){++registrations;if(throw_register)throw std::bad_alloc();return register_ok;}
};
struct Fixture {
    RegistrationFakeControl server{L"registration-allocation-test.example.com"},user{std::wstring(80,L'a')},port{L"5061"},password,ca{std::wstring(150,L'c')},tls;
    std::wstring input=std::wstring(150,L'i'),output=std::wstring(150,L'o');
    Fixture(){password.password=true;for(char c:std::string(credential))password.text.push_back(static_cast<wchar_t>(c));}
    axv::NativeRegistrationControls controls(){return {&server,&user,&port,&password,&ca,&tls};}
};
unsigned failures=0;
void check(bool yes,const char* message){if(!yes){++failures;std::fprintf(stderr,"%s\n",message);}}
axv::NativeRegistrationResult run(Fixture& f,Engine& e,long nth=-1){
    const unsigned wide_before=wide_secret_wipes,reads_before=password_reads;
    const bool had_secret=!f.password.text.empty();
    fail_after=nth;armed=true;
    axv::NativeRegistrationResult result=axv::NativeRegistrationResult::PreparationFailed;
    try{result=axv::register_native_account_ui(e,f.controls(),f.input,f.output);}
    catch(...){armed=false;fail_after=-1;check(false,"exception escaped UI registration helper");return result;}
    armed=false;fail_after=-1;
    if(f.password.reads)check(f.password.text.empty(),"password field not cleared after read");
    if(password_reads>reads_before && had_secret)check(wide_secret_wipes>wide_before,"UTF16 secret not securely wiped");
    if(result==axv::NativeRegistrationResult::PreparationFailed)
        check(f.password.text.empty(),"preparation failure did not clear password field");
    return result;
}
}
int main(){
    using R=axv::NativeRegistrationResult;
    {Fixture f;Engine e;check(run(f,e)==R::Requested,"valid registration failed");check(e.registrations==1,"registration missing");}
    {Fixture f;Engine e;f.server.text.clear();check(run(f,e)==R::InvalidAccount,"invalid account result");check(e.registrations==0,"invalid account registered");check(f.password.reads==0,"invalid account read password");check(!f.password.text.empty(),"invalid account discarded password");}
    for(const auto& port: {std::wstring(L"000000000005060x"),std::wstring(L"000000000005060")+std::wstring(112,L'0')+L"x",std::wstring(123,L'0')+L"5060x",std::wstring(123,L'0')+L"65536"}){
        Fixture f;Engine e;f.port.text=port;
        check(run(f,e)==R::InvalidAccount,"full invalid port was truncated before validation");
        check(e.configurations==0 && e.registrations==0,"invalid full port reached engine");
        check(f.password.reads==0 && !f.password.text.empty(),"invalid full port consumed password");
    }
    {Fixture f;Engine e;f.port.text=std::wstring(124,L'0')+L"5060";
        check(run(f,e)==R::Requested,"valid 128-character zero-padded port rejected");
        check(e.configurations==1 && e.registrations==1 && e.configured_port==5060,"valid full port did not reach engine intact");}
    {Fixture f;Engine e;f.password.text.clear();check(run(f,e)==R::Refused,"empty password result");check(e.configurations==0 && e.registrations==0,"empty password reached engine");}
    {Fixture f;Engine e;e.configure_ok=false;check(run(f,e)==R::Refused,"configuration refusal result");check(e.registrations==0,"configuration refusal registered");}
    {Fixture f;Engine e;e.register_ok=false;check(run(f,e)==R::Refused,"registration refusal result");}
    {Fixture f;Engine e;conversion_fail=true;check(run(f,e)==R::Refused,"UTF8 failure result");conversion_fail=false;check(e.registrations==0,"UTF8 failure registered");}
    {Fixture f;Engine e;conversion_fail_second=true;check(run(f,e)==R::Refused,"UTF8 conversion second call failure result");conversion_fail_second=false;check(e.registrations==0,"UTF8 second call failure registered");}
    {Fixture f;Engine e;e.throw_configure=true;check(run(f,e)==R::PreparationFailed,"configure exception result");check(e.registrations==0,"throwing configure registered");}
    {Fixture f;Engine e;e.throw_register=true;check(run(f,e)==R::PreparationFailed,"register exception result");}
    unsigned secret_attempts=0,failed_attempts=0,successful_attempts=0,failures_before_read=0,failures_after_read=0;
    for(long nth=0;nth<80;++nth){
        Fixture f;Engine e;const unsigned before=password_reads,injections_before=injected_failures;
        const auto result=run(f,e,nth);
        if(password_reads>before){++secret_attempts;check(wipes>0,"secret read without buffer wipe");}
        if(injected_failures>injections_before){
            ++failed_attempts;
            check(result==R::PreparationFailed,"injected allocation failure was not reported");
            check(e.registrations==0,"allocation failure registered");
            if(password_reads==before)++failures_before_read;else ++failures_after_read;
        }else{
            ++successful_attempts;
            check(result==R::Requested && e.registrations==1,"beyond-end allocation attempt failed");
        }
    }
    check(secret_attempts>0,"fault sweep never reached password preparation");
    check(failed_attempts>0,"fault sweep injected no failures");
    check(successful_attempts>0,"fault sweep did not reach beyond-end success");
    check(failures_before_read>0 && failures_after_read>0,"fault sweep missed preparation or secret allocation failures");
    check(unwiped_frees==0,"credential allocation freed without erasure");
    return failures?1:0;
}
