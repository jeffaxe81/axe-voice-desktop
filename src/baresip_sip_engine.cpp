#include "baresip_sdk_headers.h"
#include "baresip_sip_engine.h"
#include "native_account_config.h"
#include "baresip_native_libre_reactor.h"
#include "baresip_sdk_call_port.h"
#include "sip_event_projection.h"
#include <atomic>
#include <mutex>
#include <chrono>
#include <utility>
extern "C" { extern const struct mod_export exports_g711, exports_wasapi; }
namespace axv {
namespace {
void erase(std::string& s) { volatile char* p=s.empty()?nullptr:&s[0]; for(std::size_t i=0;i<s.size();++i)p[i]=0; s.clear(); }
std::uint64_t now_ms() {return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
}
struct BaresipSipEngine::Impl {
    enum class Action {None,Dial,Answer,Hangup};
    mutable std::mutex mutex;
    SipAccount account;
    NativeAccountConfig config;
    std::string password,destination;
    Action action=Action::None;
    SipRuntimeObservation view{RuntimeProcess::Stopped,SipStatus::Disconnected};
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
    void fail() { failed=true; adapter.stop(); if(owned){ua_stop_all(true);owned=nullptr;} std::lock_guard<std::mutex> lock(mutex); view={RuntimeProcess::Failed,SipStatus::Failed}; action=Action::None; destination.clear(); }
    void begin() {
        Impl* expected=nullptr;
        if(!process_owner.compare_exchange_strong(expected,this)){fail();return;}
        claimed=true;
        log_enable_stdout(false); log_enable_info(false); log_enable_debug(false);
        // Configure entirely from memory; no account or password file is created.
        if(conf_configure_buf(reinterpret_cast<const uint8_t*>(config.config.data()),config.config.size())){fail();return;}
        core=true; // close partially initialized core too
        if(baresip_init(conf_config())){fail();return;}
        ua_initialized=true;
        if(ua_init("AXE-Voice-candidate",!account.use_tls,false,account.use_tls) ||
           mod_add(&g711_module,&exports_g711) || mod_add(&wasapi_module,&exports_wasapi) ||
           ua_alloc(&owned,config.aor.c_str())){fail();return;}
        const int auth=account_set_auth_pass(ua_account(owned),password.c_str());
        erase(password);
        if(auth || account_set_regint(ua_account(owned),600) || ua_update_account(owned) || !adapter.start(owned)) {fail();return;}
        deadline=now_ms()+timeout_ms;
        if(ua_register(owned)) fail();
    }
    void tick() {
        if(failed || !owned) return;
        auto observed=projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain());
        if(!adapter.active() || observed.evidence!=SipEvidence::VerifiedNativeApi ||
           observed.sip==SipStatus::Failed || (!adapter.native_register_ok_seen() && now_ms()>=deadline)) {fail();return;}
        if(adapter.native_register_ok_seen()) deadline=UINT64_MAX;
        else if(deadline==UINT64_MAX) deadline=now_ms()+timeout_ms;
        Action pending; std::string uri;
        {
            std::lock_guard<std::mutex> lock(mutex);
            view=observed; pending=std::exchange(action,Action::None); uri=std::move(destination); destination.clear();
        }
        if(!adapter.native_register_ok_seen()) return;
        BaresipSdkCallPort calls(adapter);
        if(pending==Action::Dial && observed.sip==SipStatus::Registered && !observed.call_id) {
            if(ua_connect(owned,nullptr,nullptr,uri.c_str(),VIDMODE_OFF)) fail();
        } else if(pending==Action::Answer && observed.sip==SipStatus::Ringing) {
            if(!calls.answer(observed.call_id)) fail();
        } else if(pending==Action::Hangup && observed.call_id) {
            if(!calls.hangup(observed.call_id)) fail();
        }
    }
    void end() {
        // No SDK callback can race this: all SDK work is serialized by re_main.
        adapter.stop();
        if(ua_initialized){ ua_stop_all(true); ua_close(); ua_initialized=false; }
        owned=nullptr;
        if(claimed){conf_close(); if(core)baresip_close(); mod_close(); core=false; process_owner.store(nullptr); claimed=false;}
        erase(password);
        std::lock_guard<std::mutex> lock(mutex); view={RuntimeProcess::Stopped,SipStatus::Disconnected}; action=Action::None; destination.clear();
    }
    bool submit(Action a,std::string uri={}) {
        {std::lock_guard<std::mutex> lock(mutex);
         if(view.process!=RuntimeProcess::Running || action!=Action::None ||
            (a==Action::Dial && view.sip!=SipStatus::Registered) ||
            (a==Action::Answer && view.sip!=SipStatus::Ringing) ||
            (a==Action::Hangup && !view.call_id))return false;
         action=a;destination=std::move(uri);}
        // Native periodic timer is the fallback if the wake notification fails.
        (void)reactor.wake(); return true;
    }
};
BaresipSipEngine::BaresipSipEngine():impl_(std::make_unique<Impl>()){}
BaresipSipEngine::~BaresipSipEngine(){stop();}
SipRuntimeObservation BaresipSipEngine::observation()const {std::lock_guard<std::mutex> lock(impl_->mutex);return impl_->view;}
SipStatus BaresipSipEngine::status()const{return observation().sip;}
bool BaresipSipEngine::configure(const SipAccount&){return false;} // explicit authentication is required
bool BaresipSipEngine::configure_authenticated(const SipAccount& a,const std::wstring& ca,std::string password,unsigned timeout) {
    const auto cfg=native_account_config(a,ca);
    if(impl_->reactor.snapshot().running || !cfg.valid || password.empty() || password.size()>1024 || password.find('\0')!=std::string::npos || timeout<100 || timeout>60000) {erase(password);return false;}
    impl_->account=a;impl_->config=cfg;erase(impl_->password);impl_->password=password;erase(password);impl_->configured=true;impl_->timeout_ms=timeout;return true;
}
bool BaresipSipEngine::register_account(){
    if(!impl_->configured || impl_->reactor.snapshot().running)return false;
    impl_->configured=false;impl_->failed=false;
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->view={RuntimeProcess::Running,SipStatus::Registering};}
    return impl_->reactor.start();
}
bool BaresipSipEngine::start_call(const std::wstring& s){const auto uri=native_call_destination(s,impl_->account);return !uri.empty()&&impl_->submit(Impl::Action::Dial,uri);}
bool BaresipSipEngine::answer(){return impl_->submit(Impl::Action::Answer);}
void BaresipSipEngine::hangup(){(void)impl_->submit(Impl::Action::Hangup);}
void BaresipSipEngine::stop(){
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->view={RuntimeProcess::Stopping,SipStatus::Disconnected};impl_->action=Impl::Action::None;}
    impl_->reactor.stop();impl_->configured=false;erase(impl_->password);
    {std::lock_guard<std::mutex> lock(impl_->mutex);impl_->view={RuntimeProcess::Stopped,SipStatus::Disconnected};}
}
std::wstring BaresipSipEngine::diagnostic()const {
    switch(status()) {
    case SipStatus::Registered:return L"Registro SIP confirmado por callback nativo. Midia ainda nao homologada.";
    case SipStatus::Registering:return L"Registro SIP em andamento; aguardando a central.";
    case SipStatus::Failed:return L"Falha SIP/TLS/audio ou prazo expirado. Confira conta, CA e central; pare o motor antes de tentar novamente.";
    default:return L"Motor nativo candidato; chamadas e headset dependem de homologacao.";
    }
}
}
