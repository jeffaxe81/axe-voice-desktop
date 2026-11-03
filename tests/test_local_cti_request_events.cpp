#include "../src/local_cti_requests.h"
#include <cstdio>
#include <string>
#include <cstdint>
#define CHECK(c) do { if(!(c)){std::fprintf(stderr,"CTI transition check %d: %s\n",__LINE__,#c);return 1;} } while(false)
int main() {
    using namespace axv;
    CtiRequestQueue q;
    CHECK(q.events_after(0).events.empty());
    CHECK(q.events_after(0).latest_sequence==0);
    const std::string retry_key="client_0123456789abcdef";
    auto created=q.submit_idempotent(L"confidential-1001",retry_key,true);
    CHECK(created.outcome==CtiSubmitOutcome::Created && created.id==1);
    auto page=q.events_after(0);
    CHECK(page.events.size()==1 && page.events[0].sequence==1);
    CHECK(page.events[0].request_id==1 && page.events[0].state==CtiRequestState::Queued);
    const auto conflict=q.submit_idempotent(L"do-not-dial-again",retry_key,false);
    CHECK(conflict.outcome==CtiSubmitOutcome::KeyConflict);
    const auto repeated=q.submit_idempotent(L"confidential-1001",retry_key,false);
    CHECK(repeated.outcome==CtiSubmitOutcome::Replayed);
    CHECK(q.events_after(0).events.size()==1);
    CHECK(q.submit_idempotent(L"1002","different_12345678",false).outcome==
          CtiSubmitOutcome::NotReady);
    CHECK(q.events_after(0).events.size()==1);
    CHECK(!q.submit(L"another-call"));
    CHECK(q.events_after(0).latest_sequence==1);

    auto request=q.take();
    CHECK(request && request->id==1);
    page=q.events_after(1);
    CHECK(page.events.size()==1 && page.events[0].sequence==2 &&
          page.events[0].state==CtiRequestState::Dispatching);
    CHECK(!q.take());
    CHECK(!q.complete(666,true));
    CHECK(q.cancel(1)==CtiCancelOutcome::TooLate);
    CHECK(q.events_after(0).events.size()==2);
    CHECK(q.complete(1,true));
    CHECK(!q.complete(1,true));
    page=q.events_after(2);
    CHECK(page.events.size()==1 && page.events[0].sequence==3 &&
          page.events[0].state==CtiRequestState::Submitted);
    CHECK(q.events_after(3).events.empty());

    auto cancel_id=q.submit(L"confidential-2002");
    CHECK(cancel_id && *cancel_id==2);
    CHECK(q.cancel(2)==CtiCancelOutcome::Cancelled);
    CHECK(q.cancel(2)==CtiCancelOutcome::TooLate);
    CHECK(!q.take());
    page=q.events_after(3);
    CHECK(page.events.size()==2 &&
          page.events[0].state==CtiRequestState::Queued &&
          page.events[1].state==CtiRequestState::Cancelled);
    CHECK(page.events[0].request_id==2 && page.events[1].request_id==2);
    auto rejected=q.submit(L"confidential-3003");
    CHECK(rejected && *rejected==3 && q.take()->id==3);
    CHECK(q.complete(3,false));
    page=q.events_after(5);
    CHECK(page.events.size()==3 &&
          page.events[2].state==CtiRequestState::Rejected);
    const auto serialized=cti_request_events_json(q.events_after(0));
    CHECK(serialized.find("\"request_id\":1")!=std::string::npos);
    CHECK(serialized.find("\"status\":\"submitted\"")!=std::string::npos);
    CHECK(serialized.find("\"status\":\"cancelled\"")!=std::string::npos);
    CHECK(serialized.find("confidential")==std::string::npos);
    CHECK(serialized.find(retry_key)==std::string::npos);
    CHECK(serialized.find("destination")==std::string::npos);
    CHECK(serialized.find("sip")==std::string::npos);
    // Server-side filtering exposes only transitions of the selected ID.
    const auto only_first=q.events_after_for_request(1,0);
    CHECK(only_first && only_first->events.size()==3);
    CHECK(only_first->events[0].state==CtiRequestState::Queued);
    CHECK(only_first->events[1].state==CtiRequestState::Dispatching);
    CHECK(only_first->events[2].state==CtiRequestState::Submitted);
    for(const auto& e:only_first->events)CHECK(e.request_id==1);
    const auto only_second=q.events_after_for_request(2,0);
    CHECK(only_second && only_second->events.size()==2);
    CHECK(only_second->events[1].state==CtiRequestState::Cancelled);
    CHECK(!q.events_after_for_request(999,0));
    CHECK(q.events_after_for_request(1,3)->events.empty());
    const auto scoped_json=cti_request_events_json(*only_first);
    CHECK(scoped_json.find("\"request_id\":2")==std::string::npos);
    CHECK(scoped_json.find("\"request_id\":3")==std::string::npos);
    CHECK(scoped_json.find("confidential")==std::string::npos);

    // One event per real state change, and bounded independently from the
    // 32-item result history. No replay event or event from failed commands.
    CtiRequestQueue many;
    for(int i=0;i<30;++i) {
        auto id=many.submit(L"temporary");
        CHECK(id);
        CHECK(many.take()->id==*id);
        CHECK(many.complete(*id,true));
    }
    page=many.events_after(0);
    CHECK(page.latest_sequence==90);
    CHECK(page.events.size()==CtiRequestQueue::event_capacity);
    CHECK(page.oldest_sequence==27);
    CHECK(page.gap);
    CHECK(many.events_after(26).events.size()==64 && !many.events_after(26).gap);
    CHECK(many.events_after(89).events.size()==1);
    CHECK(many.events_after(90).events.empty());
    CHECK(many.events_after(999).events.empty());
    CHECK(many.events_after(0).events.front().sequence==27);
    CHECK(cti_request_events_json(many.events_after(0)).find("\"gap\":true")!=std::string::npos);
    // The first request is still in the 32-result registry but its events
    // have been evicted from the separate 64-event journal.
    const auto lost_scope=many.events_after_for_request(1,0);
    CHECK(lost_scope && lost_scope->gap && lost_scope->events.empty());
    const auto last_scope=many.events_after_for_request(30,0);
    CHECK(last_scope && last_scope->gap && last_scope->events.size()==3);
    CHECK(last_scope->events[0].sequence==88);
    CHECK(!many.events_after_for_request(31,0));
    many.clear();
    CHECK(!many.events_after_for_request(1,0));
    CHECK(many.events_after(0).events.empty());
    CHECK(many.events_after(0).latest_sequence==90);
    const auto new_id=many.submit(L"after_restart");
    CHECK(new_id && *new_id==31);
    CHECK(many.events_after(90).events.size()==1);
    CHECK(many.events_after(90).events[0].sequence==91);
    CHECK(many.events_after(0).gap); // Old cursors cannot resume after clear.
    return 0;
}
