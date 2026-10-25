#pragma once
#include <string>
namespace axv {
enum class SipStatus { Unavailable, Disconnected, Registering, Registered, Calling, Ringing, InCall, Failed };
struct SipAccount {
    std::wstring server;
    std::wstring username;
    unsigned short port = 5060;
    bool use_tls = false;
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
        return L"Motor Baresip ainda não integrado; nenhuma chamada foi iniciada.";
    }
};
}
