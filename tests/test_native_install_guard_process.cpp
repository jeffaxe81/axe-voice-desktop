// Real process lifetime with a detached production inventory worker.
#include "../src/native_install_guard.h"
#include "../src/audio_inventory_async.h"
#include <cstdio>
#include <string>
namespace {
struct Handle { HANDLE value=nullptr; ~Handle(){if(value)CloseHandle(value);} };
std::wstring prefix(const std::string& id) {
    return L"Local\\AXEVoice.GuardProcessTest."+std::wstring(id.begin(),id.end());
}
int fail(int code,const char* message) {std::fprintf(stderr,"process guard %d: %s\n",code,message);return code;}
int child(const std::string& id) {
    const auto base=prefix(id);
    // Process-owned handles stay valid for the detached wait until ExitProcess.
    const HANDLE entered=OpenEventW(EVENT_MODIFY_STATE|SYNCHRONIZE,FALSE,(base+L".Entered").c_str());
    const HANDLE ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,(base+L".Ready").c_str());
    const HANDLE release=OpenEventW(SYNCHRONIZE,FALSE,(base+L".Release").c_str());
    const HANDLE exit_now=OpenEventW(SYNCHRONIZE,FALSE,(base+L".Exit").c_str());
    if(!entered||!ready||!release||!exit_now)return fail(1,"child event open");
    {
        axv::NativeInstallGuard guard{axv::NativeInstallGuardLifetime::Process};
        if(guard.acquire({base+L".Running",base+L".Setup"})!=axv::NativeInstallGuardResult::Acquired)
            return fail(2,"child marker acquisition");
        axv::AudioInventoryAsync inventory([=]{
            SetEvent(entered);
            WaitForSingleObject(release,15000);
            return axv::AudioInventory{};
        });
        if(!inventory.request()||!inventory.start_pending()||WaitForSingleObject(entered,5000)!=WAIT_OBJECT_0)
            return fail(3,"production inventory worker did not enter");
        // Owner destruction does not wait for the still-gated detached probe.
    }
    if(!SetEvent(ready)||WaitForSingleObject(exit_now,10000)!=WAIT_OBJECT_0)return fail(4,"parent exit handshake");
    ExitProcess(0); // OS must release the retained marker and terminate the probe.
}
}
int main(int argc,char** argv) {
    if(argc==3&&std::string(argv[1])=="child")return child(argv[2]);
    const auto id=std::to_string(GetCurrentProcessId());
    const auto base=prefix(id);
    Handle entered{CreateEventW(nullptr,TRUE,FALSE,(base+L".Entered").c_str())};
    Handle ready{CreateEventW(nullptr,TRUE,FALSE,(base+L".Ready").c_str())};
    Handle release{CreateEventW(nullptr,TRUE,FALSE,(base+L".Release").c_str())};
    Handle exit_now{CreateEventW(nullptr,TRUE,FALSE,(base+L".Exit").c_str())};
    if(!entered.value||!ready.value||!release.value||!exit_now.value)return fail(5,"parent event creation");
    wchar_t path[32768]{};
    const DWORD length=GetModuleFileNameW(nullptr,path,32768);
    if(!length||length>=32768)return fail(6,"child executable path");
    std::wstring command=L"\""+std::wstring(path)+L"\" child "+std::wstring(id.begin(),id.end());
    STARTUPINFOW startup{};startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    if(!CreateProcessW(path,command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process))return fail(7,"child launch");
    Handle child_process{process.hProcess},child_thread{process.hThread};
    const bool reached=WaitForSingleObject(ready.value,10000)==WAIT_OBJECT_0;
    bool held=false;
    {Handle marker{OpenMutexW(SYNCHRONIZE,FALSE,(base+L".Running").c_str())};held=marker.value!=nullptr;}
    const bool probe_blocked=WaitForSingleObject(entered.value,0)==WAIT_OBJECT_0&&WaitForSingleObject(release.value,0)==WAIT_TIMEOUT;
    SetEvent(exit_now.value);
    const bool exited=WaitForSingleObject(child_process.value,10000)==WAIT_OBJECT_0;
    DWORD code=1;GetExitCodeProcess(child_process.value,&code);
    if(!reached||!exited||code!=0)return fail(8,"child lifecycle failed");
    if(!probe_blocked)return fail(9,"probe was not gated during owner teardown");
    if(!held)return fail(10,"Running marker disappeared after owners died while detached probe survived");
    Handle marker{OpenMutexW(SYNCHRONIZE,FALSE,(base+L".Running").c_str())};
    if(marker.value||GetLastError()!=ERROR_FILE_NOT_FOUND)return fail(11,"OS did not release marker at process exit");
    std::puts("Process marker survived owner destruction with blocked inventory probe; OS released marker at child exit");
    return 0;
}
