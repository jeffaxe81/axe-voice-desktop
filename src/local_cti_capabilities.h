#pragma once
#include "local_cti_events.h"
#include "local_cti_requests.h"
#include <string>

namespace axv {

// Document implemented API behavior, not a particular SIP account or call.
// Keep this independent of dynamic sessions to avoid leaking call metadata.
inline std::string local_cti_capabilities_json() {
    return std::string(R"({"api_version":"1","transport":"http_loopback_ipv4","requires_bearer_token":true,"cors":false,"features":{"click_to_call":true,"request_tracking":true,"request_events":true,"scoped_request_events":true,"cancel_queued_request":true,"idempotency_key":true,"sip_request_correlation":false,"call_completion_confirmation":false,"remote_webhooks":false,"sip_call_control_api":false},"limits":{"inflight_requests":1,"request_history":)")
        + std::to_string(CtiRequestQueue::capacity)
        + ",\"request_event_history\":" + std::to_string(CtiRequestQueue::event_capacity)
        + ",\"sip_event_history\":" + std::to_string(CtiEventJournal::capacity)
        + R"(,"idempotency_key_min_length":16,"idempotency_key_max_length":64},"semantics":{"submitted":"engine_accepted_not_connected","request_ids":"ephemeral_process_local","event_cursors":"ephemeral_process_local"}})";
}
} // namespace axv
