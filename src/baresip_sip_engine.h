#pragma once
#include "sip_engine.h"
#include "sip_runtime_observation.h"
#include "native_audio_routes.h"
#include "native_call_media.h"
#include <memory>
namespace axv {
// Candidate native implementation. All SDK pointers stay on its owner thread.
class BaresipSipEngine final : public SipEngine {
public:
    BaresipSipEngine();
    ~BaresipSipEngine() override;
    SipStatus status() const override;
    bool configure(const SipAccount&) override;
    bool configure_authenticated(const SipAccount&,const std::wstring& ca_file,
                                 std::string password,unsigned timeout_ms=15000,const NativeAudioRoute& audio_route={});
    bool register_account() override;
    bool start_call(const std::wstring&) override;
    bool answer();
    bool answer_call(const SipRuntimeObservation& displayed);
    bool hangup_call(const SipRuntimeObservation& displayed);
    bool set_muted(bool desired,const NativeCallMediaState& expected);
    bool set_held(bool desired,const NativeCallMediaState& expected);
    bool transfer_call(const std::wstring& destination,const NativeCallMediaState& expected);
    bool send_dtmf(wchar_t key,const NativeCallMediaState& expected);
    NativeCallMediaState media_state() const;
    void hangup() override;
    void stop();
    SipRuntimeObservation observation() const;
    NativeAudioRoute selected_audio_route() const;
    std::wstring diagnostic() const override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
