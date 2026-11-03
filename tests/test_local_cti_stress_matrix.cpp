#include "../src/local_cti_contract.h"
#include "../src/local_cti_requests.h"
#include "../src/local_cti_events.h"
#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>

#define REQUIRE(x) do { if(!(x)) { \
    std::fprintf(stderr,"AXE CTI matrix failure bucket=%s case=%d line=%d: %s\n", \
                 bucket,case_number,__LINE__,#x); return 1; } } while(false)

int main() {
    using namespace axv;
    constexpr int per_bucket=200;
    int completed=0;
    const char* bucket="startup";
    int case_number=0;

    bucket="valid-json-and-normalization";
    for(case_number=0;case_number<per_bucket;++case_number) {
        const auto number=std::to_string(2000+case_number);
        const auto normal=parse_cti_call_json("{\"destination\":\""+number+"\"}");
        REQUIRE(normal && *normal==std::wstring(number.begin(),number.end()));
        const auto sip="sip:"+number+"@pbx.example.org";
        const auto address=parse_cti_call_json("{\"destination\":\""+sip+"\"}");
        REQUIRE(address && *address==std::wstring(sip.begin(),sip.end()));
        const auto link=std::wstring(L"axvoice://call?destination=")
            +std::wstring(number.begin(),number.end());
        REQUIRE(parse_axvoice_click_to_call_uri(link)==normal);
        ++completed;
    }

    bucket="invalid-json-and-uri-injection";
    for(case_number=0;case_number<per_bucket;++case_number) {
        const auto suffix=std::to_string(case_number);
        REQUIRE(!parse_cti_call_json(
            "{\"destination\":\"1001\",\"force\":"+suffix+"}"));
        REQUIRE(!parse_cti_call_json(
            "{\"destination\":\"1001\",\"destination\":\""+suffix+"\"}"));
        REQUIRE(!parse_axvoice_click_to_call_uri(
            L"axvoice://call?destination=1001%0A"));
        REQUIRE(!parse_axvoice_click_to_call_uri(
            L"axvoice://call?destination=1001&debug=1"));
        ++completed;
    }

    bucket="idempotency-and-private-intent";
    for(case_number=0;case_number<per_bucket;++case_number) {
        CtiRequestQueue queue;
        const auto key="request_idempotency_"+std::to_string(case_number);
        const auto created=queue.submit_idempotent(L"1001",key,true);
        REQUIRE(created.outcome==CtiSubmitOutcome::Created && created.id==1);
        const auto mismatch=queue.submit_idempotent(L"1002",key,false);
        REQUIRE(mismatch.outcome==CtiSubmitOutcome::KeyConflict);
        const auto retry=queue.submit_idempotent(L"1001",key,false);
        REQUIRE(retry.outcome==CtiSubmitOutcome::Replayed && retry.id==1);
        REQUIRE(queue.events_after(0).events.size()==1);
        REQUIRE(queue.cancel(1)==CtiCancelOutcome::Cancelled);
        REQUIRE(queue.submit_idempotent(L"1002",key,true).outcome
                ==CtiSubmitOutcome::KeyConflict);
        REQUIRE(queue.submit_idempotent(L"1001",key,false).state
                ==CtiRequestState::Cancelled);
        const auto response=cti_request_json(*queue.get(1));
        const auto history=cti_request_events_json(queue.events_after(0));
        REQUIRE(response.find("1001")==std::string::npos);
        REQUIRE(history.find(key)==std::string::npos);
        REQUIRE(history.find("1001")==std::string::npos);
        REQUIRE(!queue.take());
        ++completed;
    }

    bucket="cancel-dispatch-and-immutable-completion";
    for(case_number=0;case_number<per_bucket;++case_number) {
        CtiRequestQueue queue;
        const auto id=queue.submit(L"private-number");
        REQUIRE(id && *id==1);
        REQUIRE(!queue.submit(L"second"));
        REQUIRE(!queue.complete(2,true));
        if((case_number&1)==0) {
            REQUIRE(queue.cancel(1)==CtiCancelOutcome::Cancelled);
            REQUIRE(queue.cancel(1)==CtiCancelOutcome::TooLate);
            REQUIRE(!queue.take());
            REQUIRE(queue.events_after(0).events.size()==2);
        } else {
            const auto taken=queue.take();
            REQUIRE(taken && taken->id==1);
            REQUIRE(queue.cancel(1)==CtiCancelOutcome::TooLate);
            const bool success=(case_number%4)==1;
            REQUIRE(queue.complete(1,success));
            REQUIRE(!queue.complete(1,!success));
            REQUIRE(queue.get(1)->state==
                    (success?CtiRequestState::Submitted:CtiRequestState::Rejected));
            REQUIRE(queue.events_after(0).events.size()==3);
        }
        ++completed;
    }

    bucket="bounded-history-and-cursor-gaps";
    for(case_number=0;case_number<per_bucket;++case_number) {
        CtiRequestQueue queue;
        const int operations=35+(case_number%5);
        for(int i=0;i<operations;++i) {
            const auto id=queue.submit(L"confidential");
            REQUIRE(id && *id==static_cast<std::uint64_t>(i+1));
            REQUIRE(queue.take()->id==*id);
            REQUIRE(queue.complete(*id,true));
        }
        REQUIRE(!queue.get(1));
        REQUIRE(queue.get(static_cast<std::uint64_t>(operations)));
        REQUIRE(!queue.events_after_for_request(1,0));
        const auto events=queue.events_after(0);
        REQUIRE(events.gap && events.events.size()==CtiRequestQueue::event_capacity);
        REQUIRE(events.latest_sequence==static_cast<std::uint64_t>(operations*3));
        REQUIRE(events.oldest_sequence==events.latest_sequence-63);
        REQUIRE(queue.events_after(events.oldest_sequence-1).events.size()==64);
        REQUIRE(!queue.events_after(events.oldest_sequence-1).gap);
        const auto scoped=queue.events_after_for_request(operations,0);
        REQUIRE(scoped && scoped->events.size()==3);
        REQUIRE(cti_request_events_json(*scoped).find("confidential")==std::string::npos);
        ++completed;
    }

    bucket="sip-evidence-epochs-and-redaction";
    for(case_number=0;case_number<per_bucket;++case_number) {
        CtiEventJournal journal;
        SipRuntimeObservation observed{};
        observed.process=RuntimeProcess::Running;
        observed.session=static_cast<std::uint64_t>(case_number+1);
        observed.sip=SipStatus::InCall;
        observed.call_id=9000000000ULL+static_cast<std::uint64_t>(case_number);
        observed.last_call_id=observed.call_id;
        journal.observe(observed);
        REQUIRE(journal.snapshot().phase==CtiPhase::Unverified);
        REQUIRE(!journal.snapshot().registered && !journal.snapshot().active_call);
        observed.evidence=SipEvidence::VerifiedNativeApi;
        journal.observe(observed);
        REQUIRE(journal.snapshot().phase==CtiPhase::InCall);
        REQUIRE(journal.snapshot().registered && journal.snapshot().active_call);
        observed.session+=1;
        observed.evidence=SipEvidence::None;
        journal.observe(observed);
        REQUIRE(journal.snapshot().phase==CtiPhase::Unverified);
        const auto json=cti_events_json(journal.after(0));
        const auto snapshot=cti_snapshot_json(journal.snapshot());
        REQUIRE(json.find(std::to_string(observed.call_id))==std::string::npos);
        REQUIRE(snapshot.find(std::to_string(observed.call_id))==std::string::npos);
        REQUIRE(json.find("session")==std::string::npos);
        REQUIRE(journal.after(0).events.size()==3);
        ++completed;
    }

    bucket="concurrent-at-most-once";
    for(case_number=0;case_number<per_bucket;++case_number) {
        CtiRequestQueue queue;
        constexpr int contenders=4;
        CtiSubmitResult results[contenders]{};
        std::atomic<bool> go{false};
        std::vector<std::thread> threads;
        threads.reserve(contenders);
        const bool same_key=(case_number%2)==0;
        for(int i=0;i<contenders;++i) {
            threads.emplace_back([&,i] {
                while(!go.load(std::memory_order_acquire))
                    std::this_thread::yield();
                const auto key=same_key?std::string("parallel_shared_0123456"):
                    ("parallel_unique_0123456_"+std::to_string(i));
                results[i]=queue.submit_idempotent(L"1001",key,true);
            });
        }
        go.store(true,std::memory_order_release);
        for(auto& worker:threads)worker.join();
        int created=0,replayed=0,busy=0;
        for(const auto& r:results) {
            if(r.outcome==CtiSubmitOutcome::Created)++created;
            else if(r.outcome==CtiSubmitOutcome::Replayed)++replayed;
            else if(r.outcome==CtiSubmitOutcome::Busy)++busy;
            else REQUIRE(false);
        }
        REQUIRE(created==1);
        REQUIRE(replayed==(same_key?3:0));
        REQUIRE(busy==(same_key?0:3));
        REQUIRE(queue.events_after(0).events.size()==1);
        const auto winner=queue.take();
        REQUIRE(winner && winner->id==1 && winner->destination==L"1001");
        REQUIRE(queue.complete(1,true));
        REQUIRE(queue.get(1)->state==CtiRequestState::Submitted);
        ++completed;
    }
    constexpr int total=7*per_bucket;
    if(completed!=total) {
        std::fprintf(stderr,"CTI matrix incomplete: %d/%d\n",completed,total);
        return 1;
    }
    std::printf("AXE CTI MULTI-MODE STRESS PASSED: %d/%d deterministic scenarios "
                "(7 domains x %d); 800 worker threads joined; no SIP hardware used\n",
                completed,total,per_bucket);
    return 0;
}
