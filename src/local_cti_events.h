#pragma once
#include "sip_runtime_observation.h"
#include <cstdint>
#include <deque>
#include <limits>
#include <mutex>
#include <string>
#include <vector>

namespace axv {

// Presentation-only state. Never export SIP account, remote destination,
// Baresip pointers or native call tokens to a CTI consumer.
enum class CtiPhase {
    Offline, Unverified, Registering, Registered,
    Dialing, Ringing, InCall, Failed
};

inline const char* cti_phase_name(CtiPhase phase) {
    switch(phase) {
    case CtiPhase::Offline: return "offline";
    case CtiPhase::Unverified: return "unverified";
    case CtiPhase::Registering: return "registering";
    case CtiPhase::Registered: return "registered";
    case CtiPhase::Dialing: return "dialing";
    case CtiPhase::Ringing: return "ringing";
    case CtiPhase::InCall: return "in_call";
    case CtiPhase::Failed: return "failed";
    }
    return "unverified";
}

inline CtiPhase cti_phase(const SipRuntimeObservation& observed) {
    if (observed.process == RuntimeProcess::Stopped ||
        observed.process == RuntimeProcess::Stopping)
        return CtiPhase::Offline;
    if (observed.process == RuntimeProcess::Failed)
        return CtiPhase::Failed;
    if (!sip_proof_is_current(observed,observed.session))
        return CtiPhase::Unverified;
    switch(observed.sip) {
    case SipStatus::Registering: return CtiPhase::Registering;
    case SipStatus::Registered: return CtiPhase::Registered;
    case SipStatus::Calling: return CtiPhase::Dialing;
    case SipStatus::Ringing: return CtiPhase::Ringing;
    case SipStatus::InCall: return CtiPhase::InCall;
    case SipStatus::Failed: return CtiPhase::Failed;
    case SipStatus::Disconnected: return CtiPhase::Offline;
    case SipStatus::Unavailable: return CtiPhase::Unverified;
    }
    return CtiPhase::Unverified;
}

inline bool cti_registered(const SipRuntimeObservation& observed) {
    if (!sip_proof_is_current(observed,observed.session)) return false;
    return observed.sip==SipStatus::Registered ||
           observed.sip==SipStatus::Calling ||
           observed.sip==SipStatus::Ringing ||
           observed.sip==SipStatus::InCall;
}

struct CtiPublicSnapshot {
    CtiPhase phase = CtiPhase::Offline;
    bool registered = false;
    bool active_call = false;
    std::uint64_t sequence = 0;
};
struct CtiPublicEvent {
    std::uint64_t sequence = 0;
    CtiPhase phase = CtiPhase::Offline;
};
struct CtiEventsPage {
    std::uint64_t latest_sequence = 0;
    std::uint64_t oldest_sequence = 0;
    bool gap = false;
    std::vector<CtiPublicEvent> events;
};

// Per-process, bounded event ring. Only verified state and de-identified
// transitions are surfaced; changing call/session tokens may cause a new
// event, but those tokens are intentionally never returned to clients.
class CtiEventJournal {
public:
    static constexpr std::size_t capacity = 32;

    void observe(const SipRuntimeObservation& state) {
        const auto phase=cti_phase(state);
        const bool proven=sip_proof_is_current(state,state.session);
        const bool registered=cti_registered(state);
        const bool active=proven && state.call_id!=0 &&
            (state.sip==SipStatus::Calling || state.sip==SipStatus::Ringing ||
             state.sip==SipStatus::InCall);
        std::lock_guard<std::mutex> lock(mutex_);
        const bool change=!initialized_ || phase!=current_.phase ||
            registered!=current_.registered || active!=current_.active_call ||
            state.session!=last_session_ || state.call_id!=last_call_id_;
        if (!change) return;
        initialized_=true;
        last_session_=state.session;
        last_call_id_=state.call_id;
        current_={phase,registered,active,++sequence_};
        events_.push_back({sequence_,phase});
        if(events_.size()>capacity)events_.pop_front();
    }

    CtiPublicSnapshot snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_;
    }

    CtiEventsPage after(std::uint64_t cursor) const {
        std::lock_guard<std::mutex> lock(mutex_);
        CtiEventsPage page;
        page.latest_sequence=sequence_;
        page.oldest_sequence=events_.empty()?0:events_.front().sequence;
        page.gap=page.oldest_sequence>0 && cursor<page.oldest_sequence-1;
        for(const auto& event:events_)
            if(event.sequence>cursor)page.events.push_back(event);
        return page;
    }

private:
    mutable std::mutex mutex_;
    bool initialized_=false;
    std::uint64_t sequence_=0;
    std::uint64_t last_session_=0;
    std::uint64_t last_call_id_=0;
    CtiPublicSnapshot current_{};
    std::deque<CtiPublicEvent> events_;
};

inline bool parse_cti_event_cursor(const std::string& text,std::uint64_t& out) {
    if(text.empty() || text.size()>20) return false;
    std::uint64_t value=0;
    for(unsigned char digit:text) {
        if(digit<'0'||digit>'9') return false;
        const std::uint64_t d=static_cast<std::uint64_t>(digit-'0');
        if(value>(std::numeric_limits<std::uint64_t>::max()-d)/10) return false;
        value=value*10+d;
    }
    out=value;
    return true;
}

inline std::string cti_snapshot_json(const CtiPublicSnapshot& state) {
    return std::string("{\"phase\":\"")+cti_phase_name(state.phase)+
        "\",\"registered\":"+(state.registered?"true":"false")+
        ",\"active_call\":"+(state.active_call?"true":"false")+
        ",\"sequence\":"+std::to_string(state.sequence)+"}";
}

inline std::string cti_events_json(const CtiEventsPage& page) {
    std::string result="{\"latest_sequence\":"+std::to_string(page.latest_sequence)+
        ",\"oldest_sequence\":"+std::to_string(page.oldest_sequence)+
        ",\"gap\":"+(page.gap?"true":"false")+",\"events\":[";
    for(std::size_t i=0;i<page.events.size();++i) {
        if(i)result+=",";
        result+="{\"sequence\":"+std::to_string(page.events[i].sequence)+
            ",\"phase\":\""+cti_phase_name(page.events[i].phase)+"\"}";
    }
    result+="]}";
    return result;
}
} // namespace axv
