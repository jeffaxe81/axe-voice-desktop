// Real Win32 controls and UTF8 conversion, synthetic engine, no SDK/network.
#include "../src/native_registration_ui.h"
#include <string>
namespace {
struct Control {
    HWND handle;
    Control(const wchar_t* type,const wchar_t* text,DWORD style=0):handle(CreateWindowExW(0,type,text,style,
        0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr)){}
    ~Control(){if(handle)DestroyWindow(handle);}
};
struct Engine {
    unsigned configurations=0,registrations=0;
    bool matched=false;
    bool configure_authenticated(const axv::SipAccount& account,const std::wstring& ca,
                                 std::string password,unsigned timeout,const axv::NativeAudioRoute& audio){
        struct Erase {std::string& value;~Erase(){if(!value.empty())SecureZeroMemory(value.data(),value.size());}} erase{password};
        ++configurations;
        matched=account.server==L"pbx.example" && account.username==L"1001" &&
                account.port==5061 && account.use_tls && ca==L"C:\\CA\\trust.pem" &&
                timeout==15000 && audio.capture_id==L"capture" && audio.playback_id==L"playback" &&
                password=="teste-\xc3\xa7-\xf0\x9f\x94\x92";
        return matched;
    }
    bool register_account(){++registrations;return true;}
};
}
int main(){
    Control server(L"EDIT",L"pbx.example"),user(L"EDIT",L"1001"),port(L"EDIT",L"5061"),
        password(L"EDIT",L"teste-\u00e7-\U0001f512"),ca(L"EDIT",L"C:\\CA\\trust.pem"),tls(L"BUTTON",L"",BS_AUTOCHECKBOX);
    axv::NativeRegistrationControls controls{server.handle,user.handle,port.handle,password.handle,ca.handle,tls.handle};
    if(!controls.server || !controls.user || !controls.port || !controls.password || !controls.ca || !controls.tls)return 1;
    SendMessageW(tls.handle,BM_SETCHECK,BST_CHECKED,0);
    Engine engine;
    using R=axv::NativeRegistrationResult;
    if(axv::register_native_account_ui(engine,controls,L"capture",L"playback")!=R::Requested)return 2;
    if(!engine.matched || engine.configurations!=1 || engine.registrations!=1 || GetWindowTextLengthW(password.handle)!=0)return 3;
    const wchar_t malformed[]={static_cast<wchar_t>(0xd800),0};
    if(!SetWindowTextW(password.handle,malformed))return 4;
    if(axv::register_native_account_ui(engine,controls,L"capture",L"playback")!=R::Refused)return 5;
    if(engine.configurations!=1 || engine.registrations!=1 || GetWindowTextLengthW(password.handle)!=0)return 6;
    if(!SetWindowTextW(server.handle,L"") || !SetWindowTextW(password.handle,L"preserve-until-form-valid"))return 7;
    if(axv::register_native_account_ui(engine,controls,L"capture",L"playback")!=R::InvalidAccount)return 8;
    if(engine.configurations!=1 || engine.registrations!=1 || GetWindowTextLengthW(password.handle)==0)return 9;
    SetWindowTextW(password.handle,L"");
    return 0;
}
