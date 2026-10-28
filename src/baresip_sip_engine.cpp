#include "baresip_sdk_headers.h"
#include "baresip_sip_engine.h"
#include "native_account_config.h"
#include "baresip_native_libre_reactor.h"
#include "baresip_sdk_call_port.h"
#include "native_session_control.h"
#include "native_audio_command_guard.h"
#include "native_transfer.h"
#include "native_destination_feedback.h"
#include "baresip_sdk_secret.h"
#include <atomic>
#include <mutex>
#include <chrono>
#include <utility>
#include <type_traits>
extern "C" { extern const struct mod_export exports_g711, exports_wasapi; }
namespace axv {
namespace {
void erase(std::string& s) { volatile char* p=s.empty()?nullptr:&s[0]; for(std::size_t i=0;i<s.size();++i)p[i]=0; s.clear(); }
static_assert(std::is_nothrow_move_assignable<SipAccount>::value &&
              std::is_nothrow_move_assignable<NativeAccountConfig>::value &&
              std::is_nothrow_move_assignable<NativeAudioRoute>::value,
              "Prepared account state must commit without throwing");
struct PasswordEraseGuard {
    std::string& value;
    ~PasswordEraseGuard(){erase(value);}
};
std::uint64_t now_ms() {return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
}
struct BaresipSipEngine::Impl {
    using Action=NativeCallAction;
    mutable std::mutex mutex;
    SipAccount account;
    bool policy_selected=false,tls_required=false;
    NativeAccountConfig config;
    NativeAudioRoute audio_route;
    NativeAudioRouteStatus audio_issue=NativeAudioRouteStatus::Ready;
    std::string password;
    NativeCallCommand command;
    NativeCallMediaState media;
    int last_hold_error=0;
    bool mute_rejected=false,dtmf_rejected=false;
    NativeCommandAdmission admission;
    SipRuntimeObservation view{RuntimeProcess::Stopped,SipStatus::Disconnected};
    int sdk_error=0;
    NativeDestinationFeedback destination_feedback;
    bool start_rejected=false,configure_rejected=false;
    std::wstring sdk_stage;
    bool configured=false,core=false,ua_initialized=false,claimed=false,failed=false;
    unsigned timeout_ms=15000;
    std::uint64_t deadline=0;
    struct ua* owned=nullptr;
    struct mod *g711_module=nullptr,*wasapi_module=nullptr;
    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle{mailbox};
    BaresipSdkEventAdapter adapter{lifecycle};
    SipEventProjection projection;
    BaresipNativeLibreReactor reactor{[this]{tick();},[this]{begin();},[this]{end();}};
    static inline std::atomic<Impl*> process_owner{nullptr};
    ~Impl(){reactor.stop();erase(password);}
    void fail() { erase(password); failed=true; adapter.stop(); if(owned){erase_sdk_account_password(ua_account(owned));ua_stop_all(true);owned=nullptr;} std::lock_guard<std::mutex> lock(mutex); view={RuntimeProcess::Failed,SipStatus::Failed}; destination_feedback.clear(); command={};media={};last_hold_error=0;mute_rejected=false;dtmf_rejected=false;audio_issue=NativeAudioRouteStatus::Ready; }
    bool checked(int error,const wchar_t* stage) {
        if(!error)return true;
        {std::lock_guard<std::mutex> lock(mutex);sdk_error=error;sdk_stage=stage;}
        fail();return false;
    }
    void publish_media(const SipRuntimeObservation& observed,BaresipSdkCallPort& calls) {
        auto current=observed.sip==SipStatus::InCall?calls.media_state(observed.call_id):NativeCallMediaState{};
        std::lock_guard<std::mutex> lock(mutex);
        destination_feedback.synchronize(observed);
        if(media.session!=current.session || media.call_id!=current.call_id)dtmf_rejected=false;
        reset_native_call_feedback(media,current,last_hold_error,mute_rejected,audio_issue);
        current.hold_request_error=last_hold_error;
        media=current;
    }
    void begin() {
        Impl* expected=nullptr;
        if(!process_owner.compare_exchange_strong(expected,this)){fail();return;}
        claimed=true;
        log_enable_stdout(false); log_enable_info(false); log_enable_debug(false);
        // Configure entirely from memory; no account or password file is created.
        if(!checked(conf_configure_buf(reinterpret_cast<const uint8_t*>(config.config.data()),config.config.size()),L"config"))return;
        core=true; // close partially initialized core too
        if(!checked(baresip_init(conf_config()),L"baresip_init"))return;
        ua_initialized=true;
        if(!checked(ua_init("AXE-Voice-candidate",!account.use_tls,false,account.use_tls),L"ua_init") ||
           !checked(mod_add(&g711_module,&exports_g711),L"g711") ||
           !checked(mod_add(&wasapi_module,&exports_wasapi),L"wasapi") ||
           !checked(ua_alloc(&owned,config.aor.c_str()),L"ua_alloc"))return;
        const int auth=account_set_auth_pass(ua_account(owned),password.c_str());
        erase(password);
        if(!checked(auth,L"auth") || !checked(account_set_regint(ua_account(owned),600),L"regint") ||
           !checked(ua_update_account(owned),L"ua_update"))return;
        if(!adapter.start(owned)){fail();return;}
        deadline=now_ms()+timeout_ms;
        checked(ua_register(owned),L"ua_register");
    }
    void tick() {
        if(failed || !owned) return;
        auto observed=project_native_sdk_batch(projection,mailbox.drain(),adapter.native_register_ok_seen());
        if(!adapter.active() || observed.evidence!=SipEvidence::VerifiedNativeApi ||
           observed.sip==SipStatus::Failed || (!adapter.native_register_ok_seen() && now_ms()>=deadline)) {fail();return;}
        if(adapter.native_register_ok_seen()) deadline=UINT64_MAX;
        else if(deadline==UINT64_MAX) deadline=now_ms()+timeout_ms;
        BaresipSdkCallPort calls(adapter);
        publish_media(observed,calls);
        NativeCallCommand pending;
        {
            std::lock_guard<std::mutex> lock(mutex);
            view=observed; pending=std::move(command); command={};
        }
        if(!adapter.native_register_ok_seen() || !native_command_is_current(pending,observed)) return;
        if(pending.action==Action::Transfer && !valid_native_transfer_uri(pending.destination))return;
        AudioInventory inventory;
        if(native_command_needs_audio_preflight(pending))
            inventory=inspect_windows_audio_devices(); // Owner thread, immediately before media creation.
        const auto decision=admit_native_audio_command(admission,pending,observed,audio_route,inventory);
        {std::lock_guard<std::mutex> lock(mutex);audio_issue=decision.audio;}
        if(!decision.allowed)return;
        if(pending.action==Action::Dial && observed.sip==SipStatus::Registered && !observed.call_id) {
            if(ua_connect(owned,nullptr,nullptr,pending.destination.c_str(),VIDMODE_OFF)) fail();
        } else if(pending.action==Action::Answer && observed.sip==SipStatus::Ringing) {
            if(!calls.answer(observed.call_id)) fail();
        } else if(pending.action==Action::Hangup && observed.call_id) {
            if(!calls.hangup(observed.call_id)) fail();
        } else if(pending.action==Action::Dtmf){
            const int error=calls.send_dtmf_info(observed.call_id,pending.destination[0]);
            {std::lock_guard<std::mutex> lock(mutex);dtmf_rejected=error!=0;}
        } else if(pending.action==Action::Transfer){
            (void)calls.transfer(observed.call_id,pending.destination); // Local submission/failure lives in SDK readback.
        } else if(pending.action==Action::SetMute){
            const bool applied=calls.set_muted(observed.call_id,pending.desired);
            {std::lock_guard<std::mutex> lock(mutex);mute_rejected=!applied;}
        } else if(pending.action==Action::SetHold){
            const auto before=calls.media_state(observed.call_id);
            if(before.available && before.local_hold!=pending.desired){
                const int error=calls.set_held(observed.call_id,pending.desired);
                {std::lock_guard<std::mutex> lock(mutex);last_hold_error=error;}
            }
        }
        publish_media(observed,calls);
    }
    void end() {
        // No SDK callback can race this: all SDK work is serialized by re_main.
        adapter.stop();
        if(owned)erase_sdk_account_password(ua_account(owned));
        if(ua_initialized){ ua_stop_all(true); ua_close(); ua_initialized=false; }
        owned=nullptr;
        if(claimed){conf_close(); if(core)baresip_close(); mod_close(); core=false; process_owner.store(nullptr); claimed=false;}
        erase(password);
        std::lock_guard<std::mutex> lock(mutex); view={RuntimeProcess::Stopped,SipStatus::Disconnected}; destination_feedback.clear(); command={};media={};last_hold_error=0;mute_rejected=false;dtmf_rejected=false;audio_issue=NativeAudioRouteStatus::Ready;
    }
    bool submit(Action a,std::string uri={},bool desired=false,const NativeCallMediaState* expected=nullptr,const SipRuntimeObservation* displayed=nullptr) {
        {std::lock_guard<std::mutex> lock(mutex);
         if(view.process!=RuntimeProcess::Running || command.action!=Action::None ||
            (a==Action::Dial && view.sip!=SipStatus::Registered) ||
            (a==Action::Answer && view.sip!=SipStatus::Ringing) ||
            (a==Action::Hangup && !view.call_id) ||
            ((a==Action::SetMute || a==Action::SetHold || a==Action::Transfer) && (!expected || !native_media_controls_current(*expected,view) || !native_media_controls_current(media,view))) ||
            (a==Action::Dtmf && (!expected || uri.size()!=1 || !valid_native_dtmf_key(static_cast<unsigned char>(uri[0])) || !native_dtmf_controls_current(*expected,view) || !native_dtmf_controls_current(media,view))) ||
            (a==Action::Transfer && media.transfer!=NativeTransferState::None) ||
            ((a==Action::Answer || a==Action::Hangup) && (!displayed || !native_displayed_call_command_current(a,*displayed,view))))return false;
         destination_feedback.command_accepted();
         command={a,view.session,view.call_id,std::move(uri),desired};}
        // Native periodic timer is the fallback if the wake notification fails.
        (void)reactor.wake(); return true;
    }
};
BaresipSipEngine::BaresipSipEngine():impl_(std::make_unique<Impl>()){}
BaresipSipEngine::~BaresipSipEngine(){stop();}
SipRuntimeObservation BaresipSipEngine::observation()const {std::lock_guard<std::mutex> lock(impl_->mutex);return impl_->view;}
NativeAudioRoute BaresipSipEngine::selected_audio_route()const {std::lock_guard<std::mutex> lock(impl_->mutex);return impl_->audio_route;}
NativeCallMediaState BaresipSipEngine::media_state()const {std::lock_guard<std::mutex> lock(impl_->mutex);return impl_->media;}
NativeSecurityStatus BaresipSipEngine::security_status()const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return native_security_status(impl_->policy_selected,impl_->tls_required,impl_->view);
}
SipStatus BaresipSipEngine::status()const{return observation().sip;}
bool BaresipSipEngine::configure(const SipAccount&){return false;} // explicit authentication is required
bool BaresipSipEngine::configure_authenticated(const SipAccount& a,const std::wstring& ca,std::string password,unsigned timeout,const NativeAudioRoute& audio) {
    PasswordEraseGuard wipe_input{password};
    if(impl_->reactor.snapshot().running)return false; // Do not disturb a live session.
    try {
        auto cfg=native_account_config(a,ca,audio);
        if(!cfg.valid || password.empty() || password.size()>1024 || password.find('\0')!=std::string::npos || timeout<100 || timeout>60000)return false;
        auto next_account=a;
        auto next_audio=audio;
        std::lock_guard<std::mutex> lock(impl_->mutex);
        // Everything except this password copy is already prepared. The account,
        // config and audio moves below use their fixed standard allocators.
        erase(impl_->password);
        impl_->password=password;
        impl_->account=std::move(next_account);impl_->config=std::move(cfg);
        impl_->audio_route=std::move(next_audio);impl_->audio_issue=NativeAudioRouteStatus::Ready;
        impl_->policy_selected=true;impl_->tls_required=a.use_tls;
        impl_->destination_feedback.clear();impl_->start_rejected=false;impl_->configure_rejected=false;
        impl_->sdk_error=0;impl_->sdk_stage.clear();
        impl_->view={RuntimeProcess::Stopped,SipStatus::Disconnected};
        impl_->configured=true;impl_->timeout_ms=timeout;
        return true;
    } catch (...) {
        // No credential or partially prepared configuration may remain armed.
        erase(impl_->password);impl_->configured=false;
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->view={RuntimeProcess::Failed,SipStatus::Failed};impl_->command={};impl_->media={};
        impl_->policy_selected=false;impl_->tls_required=false;impl_->audio_route={};
        impl_->destination_feedback.clear();impl_->sdk_error=0;impl_->sdk_stage.clear();
        impl_->start_rejected=false;impl_->configure_rejected=true;
        return false;
    }
}
bool BaresipSipEngine::register_account(){
    if(!impl_->configured || impl_->reactor.snapshot().running)return false;
    impl_->configured=false;impl_->failed=false;
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->sdk_error=0;impl_->sdk_stage.clear();impl_->destination_feedback.clear();impl_->start_rejected=false;impl_->configure_rejected=false;}
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->view={RuntimeProcess::Running,SipStatus::Registering};}
    try { if(impl_->reactor.start())return true; }
    catch (...) { /* Keep the failure path allocation-free until credentials are cleared. */ }
    // Initialization may fail before owner hooks exist. Join before touching
    // pending credentials; do not leave the UI claiming registration is pending.
    (void)impl_->reactor.stop();
    erase(impl_->password);
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->view={RuntimeProcess::Failed,SipStatus::Failed};impl_->command={};impl_->media={};impl_->start_rejected=true;}
    return false;
}
bool BaresipSipEngine::start_call(const std::wstring& s){
    auto result=native_call_destination_result(s,impl_->account);
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->destination_feedback.record(result.error,impl_->view);}
    return result && impl_->submit(Impl::Action::Dial,std::move(result.uri));
}
bool BaresipSipEngine::set_muted(bool desired,const NativeCallMediaState& expected){return impl_->submit(Impl::Action::SetMute,{},desired,&expected);}
bool BaresipSipEngine::set_held(bool desired,const NativeCallMediaState& expected){return impl_->submit(Impl::Action::SetHold,{},desired,&expected);}
bool BaresipSipEngine::transfer_call(const std::wstring& destination,const NativeCallMediaState& expected){
    auto result=native_transfer_destination_result(destination,impl_->account);
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->destination_feedback.record(result.error,impl_->view);}
    return result && impl_->submit(Impl::Action::Transfer,std::move(result.uri),false,&expected);
}
bool BaresipSipEngine::send_dtmf(wchar_t key,const NativeCallMediaState& expected){
    return valid_native_dtmf_key(key) && impl_->submit(Impl::Action::Dtmf,std::string(1,static_cast<char>(key)),false,&expected);
}
bool BaresipSipEngine::answer_call(const SipRuntimeObservation& displayed){return impl_->submit(Impl::Action::Answer,{},false,nullptr,&displayed);}
bool BaresipSipEngine::hangup_call(const SipRuntimeObservation& displayed){return impl_->submit(Impl::Action::Hangup,{},false,nullptr,&displayed);}
bool BaresipSipEngine::answer(){return answer_call(observation());}
void BaresipSipEngine::hangup(){(void)hangup_call(observation());}
void BaresipSipEngine::stop(){
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->view={RuntimeProcess::Stopping,SipStatus::Disconnected};impl_->command={};}
    impl_->reactor.stop();impl_->configured=false;erase(impl_->password);
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->view={RuntimeProcess::Stopped,SipStatus::Disconnected};impl_->media={};impl_->last_hold_error=0;impl_->mute_rejected=false;impl_->dtmf_rejected=false;impl_->audio_route={};impl_->audio_issue=NativeAudioRouteStatus::Ready;impl_->policy_selected=false;impl_->tls_required=false;impl_->destination_feedback.clear();impl_->start_rejected=false;impl_->configure_rejected=false;impl_->sdk_error=0;impl_->sdk_stage.clear();}
}
std::wstring BaresipSipEngine::diagnostic()const {
    {std::lock_guard<std::mutex> lock(impl_->mutex);
     if(impl_->sdk_error)return L"Falha SDK em "+impl_->sdk_stage+L": "+std::to_wstring(impl_->sdk_error);
     if(impl_->configure_rejected)return L"Nao foi possivel preparar a conta; senha descartada. Confira os recursos do computador e informe os dados novamente.";
     if(impl_->start_rejected)return L"Falha ao iniciar motor nativo; senha descartada. Confira os recursos do computador e tente registrar novamente.";
     if(impl_->destination_feedback.error()!=NativeDestinationError::None)return native_destination_error_message(impl_->destination_feedback.error());
     if(impl_->audio_issue!=NativeAudioRouteStatus::Ready)return native_audio_route_feedback(impl_->audio_issue);
     if(native_media_controls_current(impl_->media,impl_->view)){
         if(impl_->media.transfer==NativeTransferState::Failed)return L"Pedido de transferencia falhou. Chamada preservada; continue ou encerre.";
         if(impl_->media.transfer==NativeTransferState::Submitted)return L"Pedido de transferencia enviado ao SDK; confirmacao da central pendente.";
     }
     if(impl_->dtmf_rejected)return L"Ultimo pedido DTMF via INFO recusado pelo SDK; chamada preservada.";
     if(impl_->last_hold_error)return L"Pedido de espera/retomada falhou no SDK. Estado local nao confirma a central; retome ou encerre.";
     if(impl_->mute_rejected)return L"Nao foi possivel alterar o mudo do microfone nesta chamada.";
     if(native_media_controls_current(impl_->media,impl_->view)){
         if(impl_->media.local_hold)return L"Espera local solicitada ao SDK; confirme o comportamento na central.";
         if(impl_->media.muted)return L"Microfone em mudo no SDK; valide o silencio com o headset.";
     }}
    switch(status()) {
    case SipStatus::Registered:return L"Registro SIP confirmado por callback nativo. Midia ainda nao homologada.";
    case SipStatus::Registering:return L"Registro SIP em andamento; aguardando a central.";
    case SipStatus::Failed:return L"Falha SIP/TLS/audio ou prazo expirado. Confira conta, CA e central; pare o motor antes de tentar novamente.";
    default:return L"Motor nativo candidato; chamadas e headset dependem de homologacao.";
    }
}
}
