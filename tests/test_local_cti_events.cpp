#include "../src/local_cti_events.h"
#include <cstdio>
#include <string>
#include <cstdint>
#include <limits>
#define CHECK(expr) do{if(!(expr)){std::fprintf(stderr,"CTI event policy failure line %d: %s\n",__LINE__,#expr);return 1;}}while(false)
int main(){
    using namespace axv;
    CtiEventJournal journal;
    auto initial=journal.snapshot();
    CHECK(initial.sequence==0 && initial.phase==CtiPhase::Offline);
    CHECK(journal.after(0).events.empty());
    SipRuntimeObservation s{};
    s.process=RuntimeProcess::Running;
    s.sip=SipStatus::Registered;
    s.evidence=SipEvidence::None;
    s.session=1;
    journal.observe(s);
    CHECK(journal.snapshot().phase==CtiPhase::Unverified);
    CHECK(!journal.snapshot().registered && !journal.snapshot().active_call);
    CHECK(journal.snapshot().sequence==1);
    journal.observe(s);
    CHECK(journal.snapshot().sequence==1);
    s.evidence=SipEvidence::VerifiedNativeApi;
    journal.observe(s);
    CHECK(journal.snapshot().phase==CtiPhase::Registered);
    CHECK(journal.snapshot().registered && !journal.snapshot().active_call);
    CHECK(journal.snapshot().sequence==2);
    s.sip=SipStatus::Calling;s.call_id=61;s.last_call_id=61;
    journal.observe(s);
    CHECK(journal.snapshot().phase==CtiPhase::Dialing && journal.snapshot().active_call);
    s.sip=SipStatus::InCall;
    journal.observe(s);
    CHECK(journal.snapshot().phase==CtiPhase::InCall);
    const auto before=journal.snapshot().sequence;
    journal.observe(s);
    CHECK(journal.snapshot().sequence==before);
    s.sip=SipStatus::Registered;s.call_id=0;
    journal.observe(s);
    CHECK(!journal.snapshot().active_call);
    s.session=2;
    journal.observe(s);
    CHECK(journal.snapshot().sequence==before+2);
    s.evidence=SipEvidence::None;s.sip=SipStatus::InCall;s.call_id=91;
    journal.observe(s);
    CHECK(journal.snapshot().phase==CtiPhase::Unverified);
    CHECK(!journal.snapshot().registered && !journal.snapshot().active_call);
    std::string json=cti_snapshot_json(journal.snapshot());
    CHECK(json.find("unverified")!=std::string::npos);
    CHECK(json.find("91")==std::string::npos);
    CHECK(json.find("61")==std::string::npos);
    CHECK(json.find("session")==std::string::npos);
    s.process=RuntimeProcess::Stopped;s.sip=SipStatus::Registered;
    journal.observe(s);
    CHECK(journal.snapshot().phase==CtiPhase::Offline);
    s.process=RuntimeProcess::Failed;
    journal.observe(s);
    CHECK(journal.snapshot().phase==CtiPhase::Failed);
    const auto last=journal.snapshot().sequence;
    CHECK(journal.after(last).events.empty());
    CHECK(journal.after(last-1).events.size()==1);
    CHECK(!journal.after(last-1).gap);

    CtiEventJournal bounded;
    s.process=RuntimeProcess::Running;s.evidence=SipEvidence::VerifiedNativeApi;
    s.session=17;s.call_id=0;s.sip=SipStatus::Registered;
    for(unsigned i=0;i<75;++i){
        s.sip=(i%2)?SipStatus::Calling:SipStatus::Registered;
        s.call_id=(i%2)?i+1:0;
        bounded.observe(s);
    }
    const auto page=bounded.after(0);
    CHECK(page.latest_sequence==75);
    CHECK(page.oldest_sequence==44);
    CHECK(page.gap && page.events.size()==CtiEventJournal::capacity);
    CHECK(bounded.after(43).events.size()==32);
    CHECK(!bounded.after(43).gap);
    CHECK(bounded.after(74).events.size()==1);
    CHECK(bounded.after(100).events.empty());
    const auto events=cti_events_json(page);
    CHECK(events.find("\"gap\":true")!=std::string::npos);
    CHECK(events.find("\"sequence\":75")!=std::string::npos);
    CHECK(events.find("call_id")==std::string::npos);

    std::uint64_t cursor=999;
    CHECK(parse_cti_event_cursor("0",cursor)&&cursor==0);
    CHECK(parse_cti_event_cursor("18446744073709551615",cursor)&&cursor==std::numeric_limits<std::uint64_t>::max());
    CHECK(!parse_cti_event_cursor("18446744073709551616",cursor));
    CHECK(!parse_cti_event_cursor("",cursor));
    CHECK(!parse_cti_event_cursor("-1",cursor));
    CHECK(!parse_cti_event_cursor("1&admin=true",cursor));
    CHECK(!parse_cti_event_cursor("12 ",cursor));
    CHECK(!parse_cti_event_cursor("184467440737095516150",cursor));
    return 0;
}
