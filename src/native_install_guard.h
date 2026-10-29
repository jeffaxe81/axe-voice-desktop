#pragma once
#include <windows.h>
#include <sddl.h>
#include <array>
#include <optional>
#include <string>
namespace axv {
enum class NativeInstallGuardResult {Acquired,Busy,Failed};
enum class NativeInstallGuardLifetime {Scope,Process};
struct NativeInstallGuardNames {std::wstring application,setup;};
namespace install_guard_detail {
struct Handle {
    HANDLE value=nullptr;
    ~Handle(){if(value)CloseHandle(value);}
};
struct SidText {
    LPWSTR value=nullptr;
    ~SidText(){if(value)LocalFree(value);}
};
}
inline std::optional<NativeInstallGuardNames> native_install_guard_names() noexcept {
    try {
        install_guard_detail::Handle token;
        if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.value))return std::nullopt;
        alignas(TOKEN_USER) std::array<unsigned char,sizeof(TOKEN_USER)+SECURITY_MAX_SID_SIZE> storage{};
        DWORD bytes=0;
        if(!GetTokenInformation(token.value,TokenUser,storage.data(),static_cast<DWORD>(storage.size()),&bytes))return std::nullopt;
        const auto* user=reinterpret_cast<const TOKEN_USER*>(storage.data());
        install_guard_detail::SidText sid;
        if(!ConvertSidToStringSidW(user->User.Sid,&sid.value))return std::nullopt;
        NativeInstallGuardNames names{L"Global\\AXEVoiceNativeCandidate.Running.",L"Global\\AXEVoiceNativeCandidate.Setup."};
        names.application+=sid.value;names.setup+=sid.value;
        return names;
    }catch(...){return std::nullopt;}
}
// Publish before probing the peer. The native GUI selects Process lifetime;
// reusable guards default to scoped RAII. Never use mutex wait ownership.
class NativeInstallGuard {
public:
    explicit NativeInstallGuard(NativeInstallGuardLifetime lifetime=NativeInstallGuardLifetime::Scope) noexcept
        : lifetime_(lifetime) {}
    NativeInstallGuard(const NativeInstallGuard&)=delete;
    NativeInstallGuard& operator=(const NativeInstallGuard&)=delete;
    // Process-owned markers must also cover detached work after owner teardown.
    // Windows closes the retained handle when the process actually terminates.
    ~NativeInstallGuard(){if(lifetime_==NativeInstallGuardLifetime::Scope)release();}
    NativeInstallGuardResult acquire(const NativeInstallGuardNames& names) noexcept {
        using Result=NativeInstallGuardResult;
        if(marker_ || names.application.empty() || names.setup.empty() || names.application==names.setup)return Result::Failed;
        HANDLE candidate=CreateMutexW(nullptr,FALSE,names.application.c_str());
        if(!candidate)return Result::Failed;
        HANDLE setup=OpenMutexW(SYNCHRONIZE,FALSE,names.setup.c_str());
        const DWORD error=GetLastError();
        if(setup)CloseHandle(setup);
        if(setup || error!=ERROR_FILE_NOT_FOUND){
            CloseHandle(candidate);
            return setup?Result::Busy:Result::Failed;
        }
        marker_=candidate;
        return Result::Acquired;
    }
    void release() noexcept {if(marker_)CloseHandle(marker_);marker_=nullptr;}
private:
    const NativeInstallGuardLifetime lifetime_;
    HANDLE marker_=nullptr;
};
}
