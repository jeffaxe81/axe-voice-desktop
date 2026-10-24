#include "../src/incoming_ringtone_worker.h"
#include "../src/baresip_event_mailbox.h"
#include "../src/sip_event_projection.h"
#include <atomic>
#include <chrono>
#include <thread>

int main() {
    using namespace axv;
    using namespace std::chrono_literals;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    const auto session=mailbox.begin_session();
    if(!mailbox.enqueue({BaresipEventKind::Registering,session}) ||
       !mailbox.enqueue({BaresipEventKind::RegisterOk,session}) ||
       !mailbox.enqueue({BaresipEventKind::CallIncoming,session,1})) return 1;
    auto state=projection.observe(RuntimeProcess::Running,
        SipEvidence::VerifiedNativeApi,mailbox.drain());
    std::atomic<int> count{0};
    CancellableIncomingRingtoneSink sink(
        [&](const std::wstring&,std::uint8_t) {
            count.fetch_add(1,std::memory_order_relaxed);
            std::this_thread::sleep_for(100ms);
            return true;
        });
    IncomingRingtoneCoordinator ctrl;
    if(ctrl.update(state,session,L"pc",true,sink)!=IncomingRingtoneOutcome::Started)
        return 2;
    for(int i=0;i<100 && count.load()==0;++i)
        std::this_thread::sleep_for(10ms);
    if(count.load()==0) return 3;
    auto result=ctrl.update(state,session,L"headset",true,sink);
    if(result!=IncomingRingtoneOutcome::StopPending &&
       result!=IncomingRingtoneOutcome::Stopped) return 4;
    bool stopped=result==IncomingRingtoneOutcome::Stopped;
    for(int i=0;i<100 && !stopped;++i) {
        std::this_thread::sleep_for(10ms);
        stopped=ctrl.update(state,session,L"headset",true,sink)==IncomingRingtoneOutcome::Stopped;
    }
    if(!stopped || ctrl.stop_pending() || ctrl.playing()) return 5;
    if(ctrl.update(state,session,L"headset",true,sink)!=IncomingRingtoneOutcome::Started)
        return 6;
    if(!mailbox.enqueue({BaresipEventKind::CallEstablished,session,1})) return 7;
    state=projection.observe(RuntimeProcess::Running,
        SipEvidence::VerifiedNativeApi,mailbox.drain());
    result=ctrl.update(state,session,L"headset",true,sink);
    if(result!=IncomingRingtoneOutcome::StopPending &&
       result!=IncomingRingtoneOutcome::Stopped) return 8;
    stopped=result==IncomingRingtoneOutcome::Stopped;
    for(int i=0;i<100 && !stopped;++i) {
        std::this_thread::sleep_for(10ms);
        stopped=ctrl.update(state,session,L"headset",true,sink)==IncomingRingtoneOutcome::Stopped;
    }
    if(!stopped || ctrl.playing() || ctrl.stop_pending()) return 9;
    return 0;
}
