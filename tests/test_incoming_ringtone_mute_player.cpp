#include "../src/incoming_ringtone_player.h"
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"CHECK failed at line %d: %s\n",__LINE__,#x); std::abort(); } } while (0)
using namespace std::chrono_literals;

// Cancellation is observed cooperatively, then held behind an explicit gate.
// This lets the test replace requests while an obsolete render is still alive.
struct Renderer {
 struct Attempt { std::wstring endpoint; bool cancelled=false, release=false, returned=false; };
 std::mutex mutex;
 std::condition_variable changed;
 std::vector<Attempt> attempts;
 int concurrent=0, maximum=0;
 axv::TonePreviewState render(const std::wstring& id,const std::atomic<bool>& cancel) {
  std::unique_lock<std::mutex> lock(mutex);
  const auto index=attempts.size();
  attempts.push_back({id});
  ++concurrent; if(concurrent>maximum)maximum=concurrent;
  changed.notify_all();
  const auto deadline=std::chrono::steady_clock::now()+5s;
  while(!cancel.load()) {
   CHECK(std::chrono::steady_clock::now()<deadline);
   changed.wait_for(lock,1ms);
  }
  attempts[index].cancelled=true; changed.notify_all();
  CHECK(changed.wait_for(lock,5s,[&]{return attempts[index].release;}));
  attempts[index].returned=true; --concurrent; changed.notify_all();
  return axv::TonePreviewState::Failed;
 }
 template<class Predicate> void wait(Predicate predicate) {
  std::unique_lock<std::mutex> lock(mutex);
  CHECK(changed.wait_for(lock,5s,predicate));
 }
 void entered(std::size_t count) { wait([&]{return attempts.size()>=count;}); }
 void cancelled(std::size_t index) { wait([&]{return attempts.size()>index && attempts[index].cancelled;}); }
 void release(std::size_t index) {
  std::lock_guard<std::mutex> lock(mutex); attempts[index].release=true; changed.notify_all();
 }
 void count(std::size_t expected) {
  std::lock_guard<std::mutex> lock(mutex); CHECK(attempts.size()==expected);
 }
};

int main() {
 Renderer renderer;
 axv::IncomingRingtonePolicy policy;
 axv::SipRuntimeObservation call{axv::RuntimeProcess::Running,axv::SipStatus::Ringing,
  axv::SipEvidence::VerifiedNativeApi,10,20,20};
 axv::IncomingRingtonePlayer player([&](const std::wstring& id,const std::atomic<bool>& cancel){
  return renderer.render(id,cancel);
 },1h);
 auto sync=[&](bool muted,const std::wstring& endpoint=L"output") {
  CHECK(player.synchronize(policy.request(call,endpoint,false,muted)));
 };
 auto current=[&](std::uint64_t session,std::uint64_t token) {
  const auto snapshot=player.snapshot();
  CHECK(snapshot.request.session==session && snapshot.request.call_id==token);
  CHECK(snapshot.result==axv::TonePreviewState::Playing);
 };
 sync(false); renderer.entered(1); current(10,20);
 sync(true); renderer.cancelled(0);
 CHECK(!player.snapshot().request.active()); CHECK(player.busy());
 // A cooperative renderer may still be draining after mute. Only assert
 // silence after it has actually returned and the player has become idle.
 renderer.release(0);
 {
  std::unique_lock<std::mutex> lock(renderer.mutex);
  const auto deadline=std::chrono::steady_clock::now()+5s;
  while(player.busy()) {
   CHECK(std::chrono::steady_clock::now()<deadline);
   renderer.changed.wait_for(lock,1ms);
  }
 }
 for(int i=0;i<100;++i)sync(true);
 renderer.count(1); CHECK(player.snapshot().result==axv::TonePreviewState::Idle);
 sync(false); renderer.entered(2); current(10,20);

 // Hold the old render while intermediate call/session requests are replaced.
 // Its eventual error must neither render obsolete requests nor overwrite
 // the final request's Playing result.
 sync(true); renderer.cancelled(1);
 call.call_id=21; sync(false,L"obsolete-call");
 call.session=11; call.call_id=1; sync(false,L"obsolete-session");
 sync(true);
 call.session=12; call.call_id=2; sync(false,L"current");
 current(12,2); renderer.count(2);
 renderer.release(1); renderer.entered(3); current(12,2);
 {
  std::lock_guard<std::mutex> lock(renderer.mutex);
  CHECK(renderer.attempts[2].endpoint==L"current");
  CHECK(renderer.attempts[1].returned);
 }
 // Shutdown must cancel the live render and wait for its cooperative return.
 bool shutdown_ok=false;
 std::thread shutdown([&]{shutdown_ok=player.shutdown();});
 renderer.cancelled(2); renderer.release(2); shutdown.join();
 CHECK(shutdown_ok); CHECK(!player.busy()); CHECK(!player.snapshot().request.active());
 CHECK(!player.synchronize(policy.request(call,L"current",false,false)));
 {
  std::lock_guard<std::mutex> lock(renderer.mutex);
  CHECK(renderer.attempts.size()==3 && renderer.concurrent==0 && renderer.maximum==1);
  CHECK(renderer.attempts[2].returned);
 }
}
