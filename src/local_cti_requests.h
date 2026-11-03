#pragma once
#include "local_cti_events.h"
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace axv {

enum class CtiRequestState { Queued, Dispatching, Submitted, Rejected };
inline const char* cti_request_state_name(CtiRequestState state) {
    switch(state) {
    case CtiRequestState::Queued: return "queued";
    case CtiRequestState::Dispatching: return "dispatching";
    case CtiRequestState::Submitted: return "submitted";
    case CtiRequestState::Rejected: return "rejected";
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
};

inline std::string cti_request_json(CtiRequestResult record) {
    return "{\"request_id\":"+std::to_string(record.id)+
        ",\"status\":\""+cti_request_state_name(record.state)+"\"}";
}

class CtiRequestQueue {
public:
    static constexpr std::size_t capacity = 32;

    std::optional<std::uint64_t> submit(std::wstring destination) {
        if(destination.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(mutex_);
        if(pending_ || active_ || next_==0) return std::nullopt;
        const std::uint64_t id=next_++;
        pending_=CtiPendingRequest{id,std::move(destination)};
        records_.push_back({id,CtiRequestState::Queued});
        if(records_.size()>capacity)records_.pop_front();
        return id;
    }

    std::optional<CtiPendingRequest> take() {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!pending_ || active_) return std::nullopt;
        active_=std::move(pending_);
        pending_.reset();
        for(auto& record:records_)
            if(record.id==active_->id)record.state=CtiRequestState::Dispatching;
        return active_;
    }

    bool complete(std::uint64_t id,bool submitted) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!active_ || active_->id!=id) return false;
        active_.reset();
        for(auto& record:records_)
            if(record.id==id) {
                record.state=submitted?CtiRequestState::Submitted:CtiRequestState::Rejected;
                return true;
            }
        return false;
    }

    std::optional<CtiRequestResult> get(std::uint64_t id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for(const auto& record:records_)
            if(record.id==id)return record;
        return std::nullopt;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.reset();
        active_.reset();
        records_.clear();
        // IDs intentionally continue forward on stop/start in the same process.
    }

private:
    mutable std::mutex mutex_;
    std::uint64_t next_=1;
    std::optional<CtiPendingRequest> pending_;
    std::optional<CtiPendingRequest> active_;
    std::deque<CtiRequestResult> records_;
};
} // namespace axv
