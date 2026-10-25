#pragma once
#include "baresip_sdk_event_adapter.h"
#include "sip_command_executor.h"
#include "native_call_media.h"
#include <cerrno>
#include <cstdint>
#include <string>
#include <thread>

namespace axv {

// Experimental concrete SipNativeCommandPort for a pinned Baresip SDK.
// Used by the optional native candidate; does not itself grant provenance. Must be
// called solely on the serialized Baresip event-loop owner thread.
// The SipCommandExecutor verifies live SIP state, action and replay sequence;
// this port separately verifies native session/call ownership before SDK calls.
class BaresipSdkCallPort final : public SipNativeCommandPort {
public:
    explicit BaresipSdkCallPort(BaresipSdkEventAdapter& adapter)
        : adapter_(adapter), captured_session_(adapter.session()),
          owner_thread_(std::this_thread::get_id()) {}
    BaresipSdkCallPort(const BaresipSdkCallPort&) = delete;
    BaresipSdkCallPort& operator=(const BaresipSdkCallPort&) = delete;

    bool originate(const std::wstring& normalized_destination) override {
        if (!owns_session() || adapter_.active_call_ ||
            normalized_destination.size() < 5 ||
            normalized_destination.size() > 512) return false;

        // Do not allow native string truncation, CRLF/control or Unicode
        // conversion ambiguities through this laboratory boundary.
        // For now only explicitly formed ASCII SIP/SIPS URIs are supported.
        const bool sip = normalized_destination.compare(0, 4, L"sip:") == 0;
        const bool sips = normalized_destination.compare(0, 5, L"sips:") == 0;
        if (!sip && !sips) return false;
        const auto parsed = validate_address(normalized_destination);
        if (!parsed || parsed.normalized != normalized_destination) return false;
        std::string uri;
        uri.reserve(normalized_destination.size());
        for (wchar_t c : normalized_destination) {
            if (c <= 32 || c >= 127) return false;
            uri.push_back(static_cast<char>(c));
        }
        // This native API can synchronously emit CALL_OUTGOING events.
        // Do not touch native pointers or callback state after the call.
        return ua_connect(adapter_.owner_, nullptr, nullptr,
                          uri.c_str(), VIDMODE_OFF) == 0;
    }

    bool answer(std::uint64_t call_token) override {
        struct call* owned = resolve(call_token);
        if (!owned || !adapter_.active_incoming_ ||
            adapter_.call_established_ || adapter_.answer_sent_ ||
            adapter_.hangup_sent_) return false;
        // Record the one-time action BEFORE native reentrant callbacks.
        adapter_.answer_sent_ = true;
        // Answer may synchronously establish or close this native call.
        return ua_answer(adapter_.owner_, owned, VIDMODE_OFF) == 0;
    }

    bool hangup(std::uint64_t call_token) override {
        struct call* owned = resolve(call_token);
        if (!owned || adapter_.hangup_sent_) return false;
        // Reentrant CALL_CLOSED may clear this flag before returning.
        adapter_.hangup_sent_ = true;
        // ua_hangup() can synchronously close and free the call object.
        // Do not access 'owned' or adapter state after invoking it.
        ua_hangup(adapter_.owner_, owned, 0, nullptr);
        return true;
    }

    NativeCallMediaState media_state(std::uint64_t token) const {
        auto* owned=resolve(token);
        if(!owned || !adapter_.call_established_ || adapter_.hangup_sent_)return {};
        auto* audio=call_audio(owned);
        if(!audio)return {};
        return {captured_session_,token,true,audio_ismuted(audio),call_is_onhold(owned)};
    }
    bool set_muted(std::uint64_t token,bool desired) {
        auto* owned=resolve(token);
        if(!owned || !adapter_.call_established_ || adapter_.hangup_sent_)return false;
        auto* audio=call_audio(owned);
        if(!audio)return false;
        if(audio_ismuted(audio)!=desired)audio_mute(audio,desired);
        return audio_ismuted(audio)==desired;
    }
    int set_held(std::uint64_t token,bool desired) {
        auto* owned=resolve(token);
        if(!owned || !adapter_.call_established_ || adapter_.hangup_sent_)return EINVAL;
        // Pinned SDK changes the LOCAL flag before sending/modifying SDP.
        // Return errors to caller; readback is not remote SIP confirmation.
        if(call_is_onhold(owned)==desired)return 0;
        return call_hold(owned,desired);
    }

private:
    bool owns_session() const {
        return std::this_thread::get_id() == owner_thread_ &&
               captured_session_ != 0 && adapter_.active() &&
               adapter_.captured_.mailbox_session == captured_session_ &&
               adapter_.owner_ != nullptr && adapter_.native_register_ok_seen();
    }
    struct call* resolve(std::uint64_t token) const {
        if (!owns_session() || !token ||
            token != adapter_.active_token_ || !adapter_.active_call_)
            return nullptr;
        // Valid only during the owner's serialized event-loop invocation.
        // Do not cache, share or return this pointer outside the call.
        return const_cast<struct call*>(adapter_.active_call_);
    }

    BaresipSdkEventAdapter& adapter_;
    const std::uint64_t captured_session_;
    const std::thread::id owner_thread_;
};
} // namespace axv
