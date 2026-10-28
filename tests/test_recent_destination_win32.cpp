// Real Win32 field, shared production selection helper, no engine/network.
#include "../src/recent_destination_ui.h"
#include <cstdlib>
#include <new>
#include <string>
namespace {bool fail_allocation=false;}
void* operator new(std::size_t n){
    if(fail_allocation)throw std::bad_alloc();
    if(auto* p=std::malloc(n?n:1))return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
int main(){
    using namespace axv;
    struct Field {
        HWND handle=CreateWindowExW(0,L"EDIT",L"unchanged",0,0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        ~Field(){if(handle)DestroyWindow(handle);}
    } field;
    if(!field.handle)return 1;
    const SipRuntimeObservation current{RuntimeProcess::Running,SipStatus::Registered,SipEvidence::VerifiedNativeApi,31};
    RecentDestinations recent;
    const std::wstring typed=L"long-original-destination@example.com";
    if(!recent.remember(current,typed,true))return 2;
    const auto token=recent.at(0)->token;
    fail_allocation=true;
    const bool failed=fill_recent_destination(recent,token,current,field.handle);
    fail_allocation=false;
    wchar_t text[129]{};GetWindowTextW(field.handle,text,129);
    if(failed || std::wstring(text)!=L"unchanged" || recent.size()!=1)return 3;
    if(!fill_recent_destination(recent,token,current,field.handle))return 4;
    GetWindowTextW(field.handle,text,129);
    if(std::wstring(text)!=typed)return 5;
    auto stopped=current;stopped.process=RuntimeProcess::Stopped;
    if(fill_recent_destination(recent,token,stopped,field.handle) || recent.size()!=0)return 6;
    GetWindowTextW(field.handle,text,129);
    if(std::wstring(text)!=typed)return 7;
    if(fill_recent_destination(recent,token,current,field.handle))return 8;
    // The original typed address fits the edit; normalized sip: would be longer.
    const std::wstring boundary=std::wstring(116,L'a')+L"@example.com";
    if(boundary.size()!=128 || !recent.remember(current,boundary,true))return 9;
    if(!fill_recent_destination(recent,recent.at(0)->token,current,field.handle))return 10;
    if(GetWindowTextLengthW(field.handle)!=128)return 11;
    recent.clear();
    return recent.size()==0?0:12;
}
