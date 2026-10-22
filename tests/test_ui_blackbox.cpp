#include <windows.h>
#include <string>
#include <vector>
#include <cwchar>
#include <iostream>

namespace {
int fail(int code) {
    std::fwprintf(stderr,L"UI_BLACKBOX_FAIL exit=%d\n",code);
    return code;
}
constexpr int kCall=1001,kDestination=1002,kServer=1004,kUser=1005,kPort=1006;
constexpr int kDial=2001,kSettings=2002,kTheme=2003,kCompact=2005,kAudio=2006;
constexpr int kDigit=2100,kBackspace=2201,kClear=2202;
struct Target { DWORD pid; HWND window=nullptr; };
BOOL CALLBACK find_window(HWND hwnd,LPARAM param) {
    auto* target=reinterpret_cast<Target*>(param);
    DWORD pid=0;
    GetWindowThreadProcessId(hwnd,&pid);
    if(pid!=target->pid) return TRUE;
    wchar_t klass[128]{};
    if(GetClassNameW(hwnd,klass,128) && std::wcscmp(klass,L"AXEVoiceDesktopWindow")==0) {
        target->window=hwnd;return FALSE;
    }
    return TRUE;
}
bool clicked(HWND parent,int id) {
    HWND child=GetDlgItem(parent,id);
    if(!child) return false;
    DWORD_PTR result=0;
    return SendMessageTimeoutW(parent,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),
        reinterpret_cast<LPARAM>(child),SMTO_ABORTIFHUNG|SMTO_BLOCK,2000,&result)!=0;
}
bool shown(HWND child) {
    return child && (GetWindowLongPtrW(child,GWL_STYLE)&WS_VISIBLE)!=0;
}
std::wstring text(HWND hwnd) {
    wchar_t buffer[256]{};
    DWORD_PTR result=0;
    if(!hwnd || !SendMessageTimeoutW(hwnd,WM_GETTEXT,256,
         reinterpret_cast<LPARAM>(buffer),SMTO_ABORTIFHUNG|SMTO_BLOCK,2000,&result)) return {};
    return buffer;
}
struct Process {
    PROCESS_INFORMATION info{};
    ~Process() {
        if(info.hProcess) {
            if(WaitForSingleObject(info.hProcess,0)==WAIT_TIMEOUT) {
                TerminateProcess(info.hProcess,11);
                WaitForSingleObject(info.hProcess,3000);
            }
            CloseHandle(info.hProcess);
        }
        if(info.hThread) CloseHandle(info.hThread);
    }
};
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=2 || !argv[1] || !*argv[1]) return fail(1);
    std::wstring command=L"\""+std::wstring(argv[1])+L"\" --ui-blackbox";
    std::vector<wchar_t> mutable_cmd(command.begin(),command.end());
    mutable_cmd.push_back(L'\0');
    STARTUPINFOW start{};start.cb=sizeof(start);
    start.dwFlags=STARTF_USESHOWWINDOW;start.wShowWindow=SW_HIDE;
    Process child;
    if(!CreateProcessW(argv[1],mutable_cmd.data(),nullptr,nullptr,FALSE,0,
                       nullptr,nullptr,&start,&child.info)) return fail(2);
    Target gui{child.info.dwProcessId};
    for(int tries=0;tries<150 && !gui.window;++tries) {
        EnumWindows(find_window,reinterpret_cast<LPARAM>(&gui));
        if(!gui.window) {
            if(WaitForSingleObject(child.info.hProcess,0)==WAIT_OBJECT_0) return fail(3);
            Sleep(50);
        }
    }
    if(!gui.window) return fail(4);
    const HWND window=gui.window;
    if(!GetDlgItem(window,kDial) || !GetDlgItem(window,kSettings) ||
       !GetDlgItem(window,kAudio) || !GetDlgItem(window,kDestination) ||
       !GetDlgItem(window,kCall) || !GetDlgItem(window,kCompact) ||
       !GetDlgItem(window,kTheme)) return fail(5);
    if(!clicked(window,kSettings) || !shown(GetDlgItem(window,kServer))) return fail(6);
    if(!SetWindowTextW(GetDlgItem(window,kServer),L"pbx.example.org") ||
       !SetWindowTextW(GetDlgItem(window,kUser),L"1001") ||
       !SetWindowTextW(GetDlgItem(window,kPort),L"65536")) return fail(7);
    if(!SetWindowTextW(GetDlgItem(window,kPort),L"5060")) return fail(8);
    if(!clicked(window,kDial) || !shown(GetDlgItem(window,kDestination))) return fail(9);

    const HWND number=GetDlgItem(window,kDestination);
    int checked=0;
    for(int i=0;i<100;++i) {
        const bool valid=(i%2)==0;
        const auto destination=valid ? std::wstring(L"100")+std::to_wstring(i+100)
            : std::wstring(L"100 ")+std::to_wstring(i+100);
        if(!SetWindowTextW(number,destination.c_str())) return fail(10);
        if(IsWindowEnabled(GetDlgItem(window,kCall))) return fail(11);
        if(!clicked(window,kCall)) return fail(12);
        if(IsWindowEnabled(GetDlgItem(window,kCall))) return fail(13);
        ++checked;
    }
    if(checked!=100) return fail(14);
    if(!SetWindowTextW(number,L"1234")) return fail(15);
    SendMessageW(number,EM_SETSEL,2,2);
    if(!clicked(window,kDigit+4) || text(number)!=L"12534") return fail(16);
    SendMessageW(number,EM_SETSEL,1,4);
    if(!clicked(window,kBackspace) || text(number)!=L"14") return fail(17);
    if(!clicked(window,kClear) || !text(number).empty()) return fail(18);
    if(!clicked(window,kTheme) || !clicked(window,kTheme)) return fail(19);
    if(!clicked(window,kCompact)) return fail(20);
    RECT mini{};
    if(!GetWindowRect(window,&mini)) return fail(21);
    if(!clicked(window,kSettings) || !shown(GetDlgItem(window,kServer)) ||
       shown(GetDlgItem(window,kDestination))) return fail(22);
    RECT full{};
    if(!GetWindowRect(window,&full) || full.bottom-full.top<=mini.bottom-mini.top) return fail(23);
    if(!clicked(window,kAudio) || !shown(GetDlgItem(window,kAudio))) return fail(24);
    if(IsWindowEnabled(GetDlgItem(window,kCall))) return fail(25);
    if(!PostMessageW(window,WM_CLOSE,0,0)) return fail(26);
    if(WaitForSingleObject(child.info.hProcess,5000)!=WAIT_OBJECT_0) return fail(27);
    DWORD code=0;
    if(!GetExitCodeProcess(child.info.hProcess,&code) || code!=0) return fail(28);
    std::wcout << L"BLACKBOX: 100 destinations, external Win32 UI, no SIP or calls. PASS\n";
    return 0;
}
