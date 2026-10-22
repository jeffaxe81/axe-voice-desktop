#include "../src/account_form_feedback.h"
#include "../src/destination_feedback.h"
#include "../src/dial_authorization.h"
#include "../src/sip_runtime_session_guard.h"
#include "../src/sip_engine_factory.h"
#include <string>

int main() {
    using namespace axv;
    auto lab=create_sip_engine();
    if(!lab.engine || lab.kind!=EngineKind::Unavailable ||
       lab.engine->status()!=SipStatus::Unavailable) return 1;
    if(!evaluate_account_form(L"pbx.example.org",L"1001",L"5060").valid()) return 2;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    SipRuntimeSessionGuard guard;
    const auto& initial=guard.observe(RuntimeProcess::Stopped,SipEvidence::None,mailbox,projection);
    if(initial.session || initial.sip!=SipStatus::Unavailable) return 3;
    auto current=guard.observe(RuntimeProcess::Running,SipEvidence::None,mailbox,projection);
    if(current.session || current.sip!=SipStatus::Unavailable) return 4;
    int reviewed=0;
    for(int n=0;n<100;++n) {
        const bool valid=(n%2)==0;
        const auto typed=valid ? std::wstring(L"10")+std::to_wstring(n+100)
                               : std::wstring(L"10 ")+std::to_wstring(n+100);
        const auto feedback=evaluate_destination(typed);
        if(feedback.valid()!=valid) return 5;
        const auto denial=authorize_dial(current,current.session,lab,typed);
        if(denial.allowed()) return 6;
        if(valid && denial.reason!=DialDenied::UnverifiedSession) return 7;
        if(!valid && denial.reason!=DialDenied::InvalidDestination) return 8;
        ++reviewed;
    }
    if(reviewed!=100) return 9;
    // VerifiedNativeApi is synthetic in this test; no real SIP or registration.
    const auto session=mailbox.begin_session();
    if(!mailbox.enqueue({BaresipEventKind::Registering,session}) ||
       !mailbox.enqueue({BaresipEventKind::RegisterOk,session})) return 10;
    current=guard.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox,projection);
    if(current.sip!=SipStatus::Registered || current.session!=session) return 11;
    auto denied=authorize_dial(current,session,lab,L"1001");
    if(denied.allowed() || denied.reason!=DialDenied::EngineUnavailable) return 12;
    guard.revoke(RuntimeProcess::Stopped,mailbox,projection);
    if(mailbox.enqueue({BaresipEventKind::CallOutgoing,session})) return 13;
    current=guard.observe(RuntimeProcess::Running,SipEvidence::None,mailbox,projection);
    if(current.session || current.evidence!=SipEvidence::None) return 14;
    denied=authorize_dial(current,session,lab,L"1001");
    if(denied.allowed() || denied.reason!=DialDenied::UnverifiedSession) return 15;
    const auto newer=mailbox.begin_session();
    if(newer<=session || mailbox.enqueue({BaresipEventKind::RegisterOk,session})) return 16;
    current=guard.observe(RuntimeProcess::Running,SipEvidence::None,mailbox,projection);
    if(current.session || current.sip!=SipStatus::Unavailable) return 17;
    return 0;
}
