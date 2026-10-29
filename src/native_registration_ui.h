#pragma once
#include <windows.h>
#include "account_form_feedback.h"
#include "native_audio_routes.h"
#include <string>
namespace axv {
enum class NativeRegistrationResult {InvalidAccount,Refused,Requested,PreparationFailed};
struct NativeRegistrationControls {HWND server,user,port,password,ca,tls;};
namespace registration_ui_detail {
struct SecretScope {
    wchar_t (&wide)[257];
    std::string& utf8;
    ~SecretScope() {
        SecureZeroMemory(wide,sizeof(wide));
        if(!utf8.empty())SecureZeroMemory(utf8.data(),utf8.size());
        utf8.clear();
    }
};
}
inline const wchar_t* native_registration_result_message(NativeRegistrationResult result) noexcept {
    switch(result){
    case NativeRegistrationResult::Requested:return L"Registro solicitado; aguardando confirmacao da central.";
    case NativeRegistrationResult::InvalidAccount:return L"Conta invalida; confira servidor, usuario e porta.";
    case NativeRegistrationResult::Refused:return L"Dados, CA, senha ou audio invalidos; pare o motor ativo e confira os dados.";
    case NativeRegistrationResult::PreparationFailed:return L"Falha ao preparar registro; senha descartada. Confira os recursos do computador e informe a senha novamente.";
    }
    return L"Registro indisponivel.";
}
// Engine must erase its by-value password on every exit (the native engine does).
// Prepare every other owning argument before reading the password. The only
// argument copy at configure entry is the password, covered by these scopes
// until the callee's own guard takes over. No moved-from SSO buffer is created.
template<class Engine>
NativeRegistrationResult register_native_account_ui(Engine& engine,const NativeRegistrationControls& controls,
                                                     const std::wstring& input_id,const std::wstring& output_id) noexcept {
    wchar_t secret[257]{};
    std::string password;
    registration_ui_detail::SecretScope secrets{secret,password};
    try {
        // Account edits allow 128 characters; validate the complete visible port,
        // including zero padding or invalid suffixes, before reading the secret.
        wchar_t server[256]{},user[129]{},port[129]{},ca[260]{};
        GetWindowTextW(controls.server,server,256);GetWindowTextW(controls.user,user,129);
        GetWindowTextW(controls.port,port,129);GetWindowTextW(controls.ca,ca,260);
        const auto feedback=evaluate_account_form(server,user,port);
        if(!feedback.valid())return NativeRegistrationResult::InvalidAccount;
        const SipAccount account{server,user,feedback.port,SendMessageW(controls.tls,BM_GETCHECK,0,0)==BST_CHECKED};
        const std::wstring ca_file(ca);
        const NativeAudioRoute audio{input_id,output_id};
        GetWindowTextW(controls.password,secret,257);
        SetWindowTextW(controls.password,L"");
        const int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,secret,-1,nullptr,0,nullptr,nullptr);
        if(bytes<=1)return NativeRegistrationResult::Refused;
        password.resize(static_cast<std::size_t>(bytes),'\0');
        if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,secret,-1,password.data(),bytes,nullptr,nullptr)!=bytes)
            return NativeRegistrationResult::Refused;
        password.pop_back();
        SecureZeroMemory(secret,sizeof(secret));
        if(!engine.configure_authenticated(account,ca_file,password,15000,audio))return NativeRegistrationResult::Refused;
        return engine.register_account()?NativeRegistrationResult::Requested:NativeRegistrationResult::Refused;
    } catch (...) {
        // Win32 calls below do not allocate C++ objects or expose account data.
        SetWindowTextW(controls.password,L"");
        return NativeRegistrationResult::PreparationFailed;
    }
}
}
