#pragma once
#include "local_cti_events.h"
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace axv {

enum class CtiRequestState { Queued, Dispatching, Submitted, Rejected, Cancelled };
enum class CtiCancelOutcome { Cancelled, NotFound, TooLate };
enum class CtiSubmitOutcome { Created, Replayed, Busy, NotReady };
struct CtiSubmitResult {
    CtiSubmitOutcome outcome = CtiSubmitOutcome::Busy;
    std::uint64_t id = 0;
    CtiRequestState state = CtiRequestState::Queued;
};
inline const char* cti_request_state_name(CtiRequestState state) {
    switch(state) {
    case CtiRequestState::Queued: return "queued";
    case CtiRequestState::Dispatching: return "dispatching";
    case CtiRequestState::Submitted: return "submitted";
    case CtiRequestState::Rejected: return "rejected";
    case CtiRequestState::Cancelled: return "cancelled";
    }
    return "rejected";
}

// Identifiers are per-process and not SIP call IDs. Never report destinations,
// account information or native call pointers through the HTTP response.
struct CtiPendingRequest {
    std::uint64_t id = 0;
    std::wstring destination;
};
struct CtiRequestResult {
    std::uint64_t id = 0;
    CtiRequestState state = CtiRequestState::Queued;
    // Private per-process retry key. Never include it in cti_request_json.
    std::string idempotency_key;
};

// Lifecycle of a local CTI request, not the lifecycle of a SIP call.
struct CtiRequestTransition {
    std::uint64_t sequence = 0;
    std::uint64_t request_id = 0;
    CtiRequestState state = CtiRequestState::Queued;
};
struct CtiRequestEventsPage {
    std::uint64_t latest_sequence = 0;
    std::uint64_t oldest_sequence = 0;
    bool gap = false;
    std::vector<CtiRequestTransition> events;
};

inline std::string cti_request_events_json(const CtiRequestEventsPage& page) {
    std::string json="{\"latest_sequence\":"+std::to_string(page.latest_sequence)+
        ",\"oldest_sequence\":"+std::to_string(page.oldest_sequence)+
        ",\"gap\":"+(page.gap?"true":"false")+",\"events\":[";
    for(const auto& event:page.events) {
        if(json.back()!='[')json+=",";
        json+="{\"sequence\":"+std::to_string(event.sequence)+
            ",\"request_id\":"+std::to_string(event.request_id)+
            ",\"status\":\""+cti_request_state_name(event.state)+"\"}";
    }
    return json+"]}";
}

inline std::string cti_request_json(CtiRequestResult record) {
    return "{\"request_id\":"+std::to_string(record.id)+
        ",\"status\":\""+cti_request_state_name(record.state)+"\"}";
}

class CtiRequestQueue {
public:
    static constexpr std::size_t capacity = 32;
    static constexpr std::size_t event_capacity = 64;

    std::optional<std::uint64_t> submit(std::wstring destination) {
        if(destination.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(mutex_);
        if(pending_ || active_ || next_==0) return std::nullopt;
        const std::uint64_t id=next_++;
        pending_=CtiPendingRequest{id,std::move(destination)};
        records_.push_back({id,CtiRequestState::Queued});
        if(records_.size()>capacity)records_.pop_front();
        append_event_locked(id,CtiRequestState::Queued);
        return id;
    }

    CtiSubmitResult submit_idempotent(
        std::wstring destination,const std::string& key,bool ready) {
        if(destination.empty())return {CtiSubmitOutcome::Busy,0,CtiRequestState::Queued};
        std::lock_guard<std::mutex> lock(mutex_);
        // Check duplicates before readiness or capacity: retries never place a
        // second call, including after the first call changes SIP readiness.
        if(!key.empty()) {
            for(const auto& record:records_)
                if(record.idempotency_key==key)
                    return {CtiSubmitOutcome::Replayed,record.id,record.state};
        }
        if(!ready)return {CtiSubmitOutcome::NotReady,0,CtiRequestState::Queued};
        if(pending_ || active_ || next_==0)
            return {CtiSubmitOutcome::Busy,0,CtiRequestState::Queued};
        const std::uint64_t id=next_++;
        pending_=CtiPendingRequest{id,std::move(destination)};
        records_.push_back({id,CtiRequestState::Queued,key});
        if(records_.size()>capacity)records_.pop_front();
        append_event_locked(id,CtiRequestState::Queued);
        return {CtiSubmitOutcome::Created,id,CtiRequestState::Queued};
    }

    std::optional<CtiPendingRequest> take() {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!pending_ || active_) return std::nullopt;
        active_=std::move(pending_);
        pending_.reset();
        for(auto& record:records_) {
            if(record.id==active_->id) {
                record.state=CtiRequestState::Dispatching;
                append_event_locked(active_->id,CtiRequestState::Dispatching);
                break;
            }
        }
        return active_;
    }

    bool complete(std::uint64_t id,bool submitted) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!active_ || active_->id!=id) return false;
        active_.reset();
        for(auto& record:records_)
            if(record.id==id) {
                record.state=submitted?CtiRequestState::Submitted:CtiRequestState::Rejected;
                append_event_locked(id,record.state);
                return true;
            }
        return false;
    }

    CtiCancelOutcome cancel(std::uint64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto& record:records_) {
            if(record.id!=id)continue;
            if(!pending_ || pending_->id!=id ||
               record.state!=CtiRequestState::Queued)
                return CtiCancelOutcome::TooLate;
            pending_.reset();
            record.state=CtiRequestState::Cancelled;
            append_event_locked(id,CtiRequestState::Cancelled);
            return CtiCancelOutcome::Cancelled;
        }
        return CtiCancelOutcome::NotFound;
    }

    std::optional<CtiRequestResult> get(std::uint64_t id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for(const auto& record:records_)
            if(record.id==id)return record;
        return std::nullopt;
    }

    CtiRequestEventsPage events_after(std::uint64_t after) const {
        std::lock_guard<std::mutex> lock(mutex_);
        CtiRequestEventsPage result;
        result.latest_sequence=event_sequence_;
        result.oldest_sequence=events_.empty()?0:events_.front().sequence;
        result.gap=result.oldest_sequence>0 && after<result.oldest_sequence-1;
        for(const auto& event:events_)
            if(event.sequence>after)result.events.push_back(event);
        return result;
    }

    // Filter while holding the same queue lock. Do not disclose activity
    // for an evicted/unknown ID and never return other requests' events.
    // Cursor and gap are based on the GLOBAL event ring, conservatively:
    // a gap may flag lost events even if none belonged to this request.
    std::optional<CtiRequestEventsPage> events_after_for_request(
        std::uint64_t id,std::uint64_t after) const {
        std::lock_guard<std::mutex> lock(mutex_);
        bool known=false;
        for(const auto& record:records_)
            if(record.id==id) { known=true; break; }
        if(!known)return std::nullopt;
        CtiRequestEventsPage page;
        page.latest_sequence=event_sequence_;
        page.oldest_sequence=events_.empty()?0:events_.front().sequence;
        page.gap=page.oldest_sequence>0 && after<page.oldest_sequence-1;
        for(const auto& event:events_)
            if(event.sequence>after && event.request_id==id)
                page.events.push_back(event);
        return page;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.reset();
        active_.reset();
        records_.clear();
        events_.clear();
        // IDs and event cursors never reused after stop/start in this process.
    }

private:
    // Must run under mutex_. A retry/replay is not a new transition.
    void append_event_locked(std::uint64_t id,CtiRequestState state) {
        if(event_sequence_==std::numeric_limits<std::uint64_t>::max())return;
        events_.push_back({++event_sequence_,id,state});
        if(events_.size()>event_capacity)events_.pop_front();
    }

    mutable std::mutex mutex_;
    std::uint64_t next_=1;
    std::optional<CtiPendingRequest> pending_;
    std::optional<CtiPendingRequest> active_;
    std::deque<CtiRequestResult> records_;
    std::deque<CtiRequestTransition> events_;
    std::uint64_t event_sequence_=0;
};
} // namespace axv
