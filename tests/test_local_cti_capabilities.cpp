#include "../src/local_cti_capabilities.h"
#include <cstdio>
#include <string>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"CTI capability test line %d: %s\n",__LINE__,#x); return 1; } } while(false)
int main() {
    const auto json=axv::local_cti_capabilities_json();
    CHECK(json.find(R"("api_version":"1")")!=std::string::npos);
    CHECK(json.find(R"("transport":"http_loopback_ipv4")")!=std::string::npos);
    CHECK(json.find(R"("requires_bearer_token":true)")!=std::string::npos);
    CHECK(json.find(R"("cors":false)")!=std::string::npos);
    CHECK(json.find(R"("click_to_call":true)")!=std::string::npos);
    CHECK(json.find(R"("request_tracking":true)")!=std::string::npos);
    CHECK(json.find(R"("request_events":true)")!=std::string::npos);
    CHECK(json.find(R"("scoped_request_events":true)")!=std::string::npos);
    CHECK(json.find(R"("cancel_queued_request":true)")!=std::string::npos);
    CHECK(json.find(R"("idempotency_key":true)")!=std::string::npos);
    CHECK(json.find(R"("sip_request_correlation":false)")!=std::string::npos);
    CHECK(json.find(R"("call_completion_confirmation":false)")!=std::string::npos);
    CHECK(json.find(R"("remote_webhooks":false)")!=std::string::npos);
    CHECK(json.find(R"("sip_call_control_api":false)")!=std::string::npos);
    CHECK(json.find("\"request_history\":"+std::to_string(axv::CtiRequestQueue::capacity))!=std::string::npos);
    CHECK(json.find("\"request_event_history\":"+std::to_string(axv::CtiRequestQueue::event_capacity))!=std::string::npos);
    CHECK(json.find("\"sip_event_history\":"+std::to_string(axv::CtiEventJournal::capacity))!=std::string::npos);
    CHECK(json.find(R"("idempotency_key_min_length":16)")!=std::string::npos);
    CHECK(json.find(R"("idempotency_key_max_length":64)")!=std::string::npos);
    CHECK(json.find(R"("submitted":"engine_accepted_not_connected")")!=std::string::npos);
    CHECK(json.find("password")==std::string::npos);
    CHECK(json.find("destination")==std::string::npos);
    CHECK(json.find("call_id")==std::string::npos);
    CHECK(json.find("bearer_token")==std::string::npos || json.find(R"("requires_bearer_token":true)")!=std::string::npos);
    CHECK(json.size()<2048);
    CHECK(json.front()=='{' && json.back()=='}');
    return 0;
}
