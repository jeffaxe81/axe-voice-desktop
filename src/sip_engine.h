#pragma once
#include <string>
#include "native_codec_mode.h"
namespace axv {
enum class SipStatus { Unavailable, Disconnected, Registering, Registered, Calling, Ringing, InCall, Failed };
struct SipAccount {
    std::wstring server;
    std::wstring username;
    unsigned short port = 5060;
    bool use_tls = false;
    NativeCodecMode codec_mode = NativeCodecMode::Automatic;
};
class SipEngine {
public:
    virtual ~SipEngine() = default;
    virtual SipStatus status() const = 0;
    virtual bool configure(const SipAccount& account) = 0;
    virtual bool register_account() = 0;
    virtual bool start_call(const std::wstring& destination) = 0;
    virtual void hangup() = 0;
    virtual std::wstring diagnostic() const = 0;
};
class UnavailableSipEngine final : public SipEngine {
public:
    SipStatus status() const override { return SipStatus::Unavailable; }
    bool configure(const SipAccount&) override { return false; }
    bool register_account() override { return false; }
    bool start_call(const std::wstring&) override { return false; }
    void hangup() override {}
    std::wstring diagnostic() const override {
        return L"Esta compilacao nao inclui motor SIP. Instale o pacote nativo AXE-Voice-Native-Candidate-Setup para registrar.";
    }
};
}
