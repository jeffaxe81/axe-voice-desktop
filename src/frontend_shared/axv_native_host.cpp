// SDK-backed WinUI3 native host. No C# password, no HTTP, no second SIP stack.
// This DLL is built ONLY with the audited native Baresip/libre SDK profile.
#include "axv_native_host.h"
#include "axv_saved_account_policy.h"
#include "../baresip_sip_engine.h"
#include "../voice_application_service.h"
#include "../native_codec_mode.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wincred.h>
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace axv {
namespace {
constexpr wchar_t kRegistryPath[]=L"Software\\AXEVoice\\Desktop";
constexpr wchar_t kCredentialTarget[]=L"AXEVoice/SIP/DefaultAccount";

// Always erase credential memory even when C++ allocation/conversion throws.
struct ScopedCredential {
    PCREDENTIALW value=nullptr;
    ScopedCredential()=default;
    ScopedCredential(const ScopedCredential&)=delete;
    ScopedCredential& operator=(const ScopedCredential&)=delete;
    ~ScopedCredential() {
        if(!value)return;
        if(value->CredentialBlob && value->CredentialBlobSize)
            SecureZeroMemory(value->CredentialBlob,value->CredentialBlobSize);
        CredFree(value);
    }
};

struct SavedAccount {
    SipAccount account;
    std::wstring ca;
    std::string secret;
    ~SavedAccount() {
        if(!secret.empty())SecureZeroMemory(secret.data(),secret.size());
        secret.clear();
    }
};

bool read_reg_text(const wchar_t* value,std::wstring& output,DWORD max_chars) {
    std::wstring buffer(static_cast<size_t>(max_chars)+1,L'\0');
    DWORD length=static_cast<DWORD>(buffer.size()*sizeof(wchar_t));
    const LONG rc=RegGetValueW(HKEY_CURRENT_USER,kRegistryPath,value,
                               RRF_RT_REG_SZ,nullptr,buffer.data(),&length);
    if(rc!=ERROR_SUCCESS || !length || length%sizeof(wchar_t))return false;
    const auto it=std::find(buffer.begin(),buffer.end(),L'\0');
    if(it==buffer.end())return false;
    output.assign(buffer.begin(),it);
    return true;
}
bool read_saved_metadata(SavedSipFields& f) {
    return read_reg_text(L"SipServer",f.server,253) &&
           read_reg_text(L"SipUser",f.user,128) &&
           read_reg_text(L"SipPort",f.port,5) &&
           read_reg_text(L"SipTransport",f.transport,10) &&
           read_reg_text(L"SipCaFile",f.ca_file,240);
}

bool read_saved_secret(const SavedSipFields& fields,std::string& output) {
    ScopedCredential credential{};
    if(!CredReadW(kCredentialTarget,CRED_TYPE_GENERIC,0,&credential.value))
        return false;
    const std::wstring identity=fields.server+L"\n"+fields.user;
    if(!credential.value->UserName || identity!=credential.value->UserName ||
       !credential.value->CredentialBlob ||
       !credential.value->CredentialBlobSize ||
       credential.value->CredentialBlobSize>256*sizeof(wchar_t) ||
       credential.value->CredentialBlobSize%sizeof(wchar_t))
        return false;
    const auto* data=reinterpret_cast<const wchar_t*>(
        credential.value->CredentialBlob);
    const int length=static_cast<int>(
        credential.value->CredentialBlobSize/sizeof(wchar_t));
    if(std::find(data,data+length,L'\0')!=data+length)return false;
    const int count=WideCharToMultiByte(
        CP_UTF8,WC_ERR_INVALID_CHARS,data,length,nullptr,0,nullptr,nullptr);
    if(count<=0 || count>1024)return false;
    output.resize(static_cast<size_t>(count));
    const int wrote=WideCharToMultiByte(
        CP_UTF8,WC_ERR_INVALID_CHARS,data,length,output.data(),
        count,nullptr,nullptr);
    if(wrote!=count){
        if(!output.empty())SecureZeroMemory(output.data(),output.size());
        output.clear();
        return false;
    }
    return true;
}
bool load_account(SavedAccount& target) {
    SavedSipFields fields{};
    if(!read_saved_metadata(fields))return false;
    auto parsed=validate_saved_sip_account(fields);
    if(parsed.status!=SavedAccountStatus::Ready)return false;
    if(parsed.account.use_tls){
        const DWORD attrs=GetFileAttributesW(parsed.ca_file.c_str());
        if(attrs==INVALID_FILE_ATTRIBUTES ||
           (attrs&FILE_ATTRIBUTE_DIRECTORY)!=0)return false;
    }
    if(!read_saved_secret(fields,target.secret))return false;
    target.account=std::move(parsed.account);
    target.ca=std::move(parsed.ca_file);
    DWORD codec=0,length=sizeof(codec);
    if(RegGetValueW(HKEY_CURRENT_USER,kRegistryPath,L"AudioCodecMode",
                    RRF_RT_REG_DWORD,nullptr,&codec,&length)==ERROR_SUCCESS &&
       valid_native_codec_mode(static_cast<NativeCodecMode>(codec)))
        target.account.codec_mode=static_cast<NativeCodecMode>(codec);
    return true;
}

struct SingleOwnerHost {
    std::mutex mutex;
    std::unique_ptr<BaresipSipEngine> engine;
};
// Process-scoped owner, never destroyed under DllMain/loader lock. The WinUI 3
// window explicitly invokes disconnect before process exit, which joins the
// libre reactor and releases all native SDK resources outside the loader lock.
SingleOwnerHost& single_host() {
    static auto* host=new SingleOwnerHost;
    return *host;
}

void fill(AxvFrontendSnapshot* out,const VoiceApplicationSnapshot& observed) {
    out->connection=static_cast<uint32_t>(observed.signal);
    out->view=static_cast<uint32_t>(observed.actions.view);
    out->can_dial=observed.actions.dial;
    out->can_answer=observed.actions.answer;
    out->can_hangup=observed.actions.hangup;
    out->can_secondary=observed.actions.secondary;
    if(sip_proof_is_current(observed.observation,
                            observed.observation.session)){
        out->session=observed.observation.session;
        out->call_id=observed.observation.call_id;
    }
}

bool action_current(const SipRuntimeObservation& current,
                    std::uint64_t session,std::uint64_t call_id) {
    return session && call_id && current.session==session &&
           current.call_id==call_id &&
           sip_proof_is_current(current,session);
}
} // namespace

int32_t native_host_get_snapshot(AxvFrontendSnapshot* out) noexcept {
    try{
        auto& host=single_host();
        std::lock_guard<std::mutex> guard(host.mutex);
        out->backend_attached=1; // SDK linked, NOT a claim of registration.
        if(!host.engine)return AXV_FRONTEND_OK;
        VoiceApplicationService<BaresipSipEngine> service(*host.engine);
        fill(out,service.snapshot(true));
        return AXV_FRONTEND_OK;
    }catch(...){return AXV_FRONTEND_ERROR_REJECTED;}
}

int32_t native_host_connect_saved() noexcept {
    try{
        auto& host=single_host();
        std::lock_guard<std::mutex> guard(host.mutex);
        if(host.engine){
            const auto current=host.engine->observation();
            if(current.process==RuntimeProcess::Running ||
               current.process==RuntimeProcess::Stopping)
                return AXV_FRONTEND_ERROR_REJECTED;
            host.engine->stop();
        }
        SavedAccount saved{};
        if(!load_account(saved))return AXV_FRONTEND_ERROR_ACCOUNT;
        if(!host.engine)host.engine=std::make_unique<BaresipSipEngine>();
        if(!host.engine->configure_authenticated(
               saved.account,saved.ca,saved.secret,15000,{}))
            return AXV_FRONTEND_ERROR_REJECTED;
        return host.engine->register_account()
            ?AXV_FRONTEND_OK:AXV_FRONTEND_ERROR_REJECTED;
    }catch(...){return AXV_FRONTEND_ERROR_REJECTED;}
}

int32_t native_host_disconnect() noexcept {
    try{
        auto& host=single_host();
        std::lock_guard<std::mutex> guard(host.mutex);
        if(host.engine)host.engine->stop();
        return AXV_FRONTEND_OK;
    }catch(...){return AXV_FRONTEND_ERROR_REJECTED;}
}
int32_t native_host_dial(const std::wstring& destination,
                         std::uint64_t expected_session) noexcept {
    try{
        auto& host=single_host();
        std::lock_guard<std::mutex> guard(host.mutex);
        if(!host.engine || !expected_session)
            return AXV_FRONTEND_ERROR_REJECTED;
        const auto current=host.engine->observation();
        if(current.session!=expected_session ||
           !sip_proof_is_current(current,expected_session))
            return AXV_FRONTEND_ERROR_REJECTED;
        VoiceApplicationService<BaresipSipEngine> service(*host.engine);
        return service.dial(current,destination)
            ?AXV_FRONTEND_OK:AXV_FRONTEND_ERROR_REJECTED;
    }catch(...){return AXV_FRONTEND_ERROR_REJECTED;}
}
int32_t native_host_answer(std::uint64_t expected_session,
                           std::uint64_t expected_call_id) noexcept {
    try{
        auto& host=single_host();
        std::lock_guard<std::mutex> guard(host.mutex);
        if(!host.engine)return AXV_FRONTEND_ERROR_REJECTED;
        const auto displayed=host.engine->observation();
        if(!action_current(displayed,expected_session,expected_call_id))
            return AXV_FRONTEND_ERROR_REJECTED;
        VoiceApplicationService<BaresipSipEngine> service(*host.engine);
        return service.answer(displayed)
            ?AXV_FRONTEND_OK:AXV_FRONTEND_ERROR_REJECTED;
    }catch(...){return AXV_FRONTEND_ERROR_REJECTED;}
}
int32_t native_host_hangup(std::uint64_t expected_session,
                           std::uint64_t expected_call_id) noexcept {
    try{
        auto& host=single_host();
        std::lock_guard<std::mutex> guard(host.mutex);
        if(!host.engine)return AXV_FRONTEND_ERROR_REJECTED;
        const auto displayed=host.engine->observation();
        if(!action_current(displayed,expected_session,expected_call_id))
            return AXV_FRONTEND_ERROR_REJECTED;
        VoiceApplicationService<BaresipSipEngine> service(*host.engine);
        return service.hangup(displayed)
            ?AXV_FRONTEND_OK:AXV_FRONTEND_ERROR_REJECTED;
    }catch(...){return AXV_FRONTEND_ERROR_REJECTED;}
}
} // namespace axv
