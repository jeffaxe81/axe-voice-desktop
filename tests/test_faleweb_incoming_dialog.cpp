// UI regression for the *actual modal Win32 preferences dialog*. No SIP I/O.
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "../src/faleweb_preferences_win32.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>

struct InboundFixture {
    std::atomic<bool> incoming{false};
    std::atomic<int> answers{0};
    std::atomic<int> declines{0};
    std::atomic<bool> passed{true};
};
static bool predicate(void* ctx) {
    return static_cast<InboundFixture*>(ctx)->incoming.load();
}
static void answer(void* ctx) {
    static_cast<InboundFixture*>(ctx)->answers.fetch_add(1);
}
static void decline(void* ctx) {
    static_cast<InboundFixture*>(ctx)->declines.fetch_add(1);
}
template<class F> static bool until(F condition, int max_ms=5000) {
    for(int n=0;n<max_ms/20;++n) {
        if(condition())return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}
static void fail(InboundFixture& x, const char* msg) {
    std::fprintf(stderr,"FAIL: %s\n",msg); x.passed=false;
}
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    InboundFixture x;
    HWND owner=CreateWindowExW(0,L"STATIC",L"AXE Incoming Prefs Smoke",
        WS_POPUP,0,0,150,120,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!owner)return 90;
    axv::FalewebPreferences data;
    data.server=L"unchanged.example.invalid";
    data.port=L"5060";
    const axv::FalewebIncomingActions callbacks{&x,&predicate,&answer,&decline};
    std::thread worker([&] {
        HWND dialog=nullptr;
        if(!until([&]{
            dialog=FindWindowW(L"#32770",L"AXE Voice - Preferencias");
            return dialog!=nullptr;
        })) {
            fail(x,"modal dialog was not created");return;
        }
        const HWND yes=GetDlgItem(dialog,axv::kPrefsIncomingAnswer);
        const HWND no=GetDlgItem(dialog,axv::kPrefsIncomingReject);
        if(!yes || !no) { fail(x,"answer/reject controls missing");goto done; }
        if(IsWindowVisible(yes)||IsWindowVisible(no))
            fail(x,"incoming controls visible without a call");

        x.incoming=true;
        if(!until([&]{return IsWindowVisible(yes)&&IsWindowVisible(no);}))
            {fail(x,"incoming call controls not revealed");goto done;}

        // Tabs must not suppress the emergency call actions.
        SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(705,BN_CLICKED),0);
        if(!IsWindowVisible(yes)||!IsWindowVisible(no))
            fail(x,"advanced preferences tab blocked incoming controls");
        SendMessageW(dialog,WM_COMMAND,
            MAKEWPARAM(axv::kPrefsIncomingAnswer,BN_CLICKED),
            reinterpret_cast<LPARAM>(yes));
        if(x.answers.load()!=1)fail(x,"answer callback not sent exactly once");
        if(x.declines.load()!=0)fail(x,"reject callback sent inadvertently");

        x.incoming=false;
        if(!until([&]{return !IsWindowVisible(yes)&&!IsWindowVisible(no);}))
            {fail(x,"stale incoming buttons remain visible");goto done;}
        SendMessageW(dialog,WM_COMMAND,
            MAKEWPARAM(axv::kPrefsIncomingAnswer,BN_CLICKED),
            reinterpret_cast<LPARAM>(yes));
        if(x.answers.load()!=1)fail(x,"stale WM_COMMAND accepted after incoming call ended");

        x.incoming=true;
        if(!until([&]{return IsWindowVisible(yes)&&IsWindowVisible(no);}))
            {fail(x,"new incoming call controls not available");goto done;}
        SendMessageW(dialog,WM_COMMAND,
            MAKEWPARAM(axv::kPrefsIncomingReject,BN_CLICKED),
            reinterpret_cast<LPARAM>(no));
        if(x.declines.load()!=1)fail(x,"reject action not sent exactly once");

done:
        SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDCANCEL,BN_CLICKED),0);
    });
    const bool saved=axv::edit_faleweb_preferences(owner,data,callbacks);
    worker.join();
    DestroyWindow(owner);
    if(saved || data.server!=L"unchanged.example.invalid" || !x.passed.load())
        return 91;
    return 0;
}
