#include "../src/local_cti_requests.h"
#include <cstdio>
#include <string>
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"CTI request test %d: %s\n",__LINE__,#v);return 1;}}while(false)
int main(){
    using namespace axv;
    CtiRequestQueue cancel_queue;
    CHECK(cancel_queue.cancel(1)==CtiCancelOutcome::NotFound);
    auto cancel_id=cancel_queue.submit(L"confidential-number");
    CHECK(cancel_id && *cancel_id==1);
    CHECK(cancel_queue.cancel(1)==CtiCancelOutcome::Cancelled);
    CHECK(cancel_queue.get(1)->state==CtiRequestState::Cancelled);
    CHECK(!cancel_queue.take());
    CHECK(cancel_queue.cancel(1)==CtiCancelOutcome::TooLate);
    CHECK(cti_request_json(*cancel_queue.get(1))==
          R"({"request_id":1,"status":"cancelled"})");
    CHECK(cti_request_json(*cancel_queue.get(1)).find("confidential")==std::string::npos);
    cancel_id=cancel_queue.submit(L"another-number");
    CHECK(cancel_id && *cancel_id==2);
    CHECK(cancel_queue.take()->id==2);
    CHECK(cancel_queue.cancel(2)==CtiCancelOutcome::TooLate);
    CHECK(cancel_queue.complete(2,true));
    CHECK(cancel_queue.cancel(2)==CtiCancelOutcome::TooLate);
    CHECK(cancel_queue.get(2)->state==CtiRequestState::Submitted);

    CtiRequestQueue queue;
    CHECK(!queue.get(1));
    CHECK(!queue.take());
    CHECK(!queue.submit(L""));
    const auto first=queue.submit(L"1001");
    CHECK(first && *first==1);
    CHECK(!queue.submit(L"1002"));
    CHECK(queue.get(1) && queue.get(1)->state==CtiRequestState::Queued);
    auto active=queue.take();
    CHECK(active && active->id==1 && active->destination==L"1001");
    CHECK(!queue.submit(L"1002")); // Inflight request is exclusive.
    CHECK(!queue.take());
    CHECK(queue.get(1)->state==CtiRequestState::Dispatching);
    CHECK(!queue.complete(2,true)); // Cannot forge a request outcome.
    CHECK(queue.complete(1,true));
    CHECK(!queue.complete(1,false)); // Completion is immutable.
    CHECK(queue.get(1)->state==CtiRequestState::Submitted);
    CHECK(cti_request_json(*queue.get(1))==
          R"({"request_id":1,"status":"submitted"})");
    const auto second=queue.submit(L"sip:1002@example.org");
    CHECK(second && *second==2);
    CHECK(queue.take()->id==2);
    CHECK(queue.complete(2,false));
    CHECK(queue.get(2)->state==CtiRequestState::Rejected);
    const auto third=queue.submit(L"1003");
    CHECK(third && *third==3);
    queue.clear();
    CHECK(!queue.take());
    CHECK(!queue.get(1));
    CHECK(!queue.get(3));
    const auto fourth=queue.submit(L"1004");
    CHECK(fourth && *fourth==4); // No ID reused after restart.
    CHECK(queue.take()->id==4);
    CHECK(queue.complete(4,true));

    // Bounded completed-result history. Neither the destination nor SIP
    // identity may enter JSON responses.
    for(int i=0;i<39;++i){
        auto id=queue.submit(L"private-number");
        CHECK(id && *id==static_cast<std::uint64_t>(i+5));
        CHECK(queue.take()->id==*id);
        CHECK(queue.complete(*id,true));
    }
    CHECK(!queue.get(4));
    CHECK(queue.get(43) && queue.get(43)->state==CtiRequestState::Submitted);
    CHECK(queue.get(12));
    const auto body=cti_request_json(*queue.get(43));
    CHECK(body.find("private")==std::string::npos);
    CHECK(body.find("sip")==std::string::npos);
    CHECK(body.find("43")!=std::string::npos);
    return 0;
}
