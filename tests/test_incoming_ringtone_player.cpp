#include "../src/incoming_ringtone_player.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <new>
#include <thread>
#include <vector>
// Deterministically exercise the existing thread-construction failure path.
// Thread-local injection cannot affect allocations in a renderer worker.
namespace { thread_local bool fail_next_allocation=false; }
void* operator new(std::size_t size){
 if(fail_next_allocation){fail_next_allocation=false;throw std::bad_alloc();}
 if(void* p=std::malloc(size?size:1))return p;
 throw std::bad_alloc();
}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"CHECK failed at line %d: %s\n",__LINE__,#x);std::abort();}}while(0)
int main(){
 using namespace std::chrono_literals;
 {
  std::atomic<bool> rendered{false};
  axv::IncomingRingtonePlayer allocation_failure([&](const std::wstring&,const std::atomic<bool>&){
   rendered=true;return axv::TonePreviewState::Played;
  },1h);
  // The one-character ID fits std::wstring's small buffer; the next
  // allocation belongs to std::thread, after the request is installed.
  fail_next_allocation=true;
  CHECK(!allocation_failure.synchronize({10,1,L"x"}));
  CHECK(!fail_next_allocation);
  CHECK(allocation_failure.snapshot().result==axv::TonePreviewState::Failed);
  CHECK(allocation_failure.synchronize({10,1,L"x"}));
  const auto retry_deadline=std::chrono::steady_clock::now()+2s;
  while(!rendered && std::chrono::steady_clock::now()<retry_deadline)std::this_thread::yield();
  CHECK(rendered);
  CHECK(allocation_failure.shutdown());
 }
 std::mutex mutex;std::condition_variable entered;
 std::vector<std::wstring> ids;
 std::atomic<int> concurrent{0},maximum{0};
 axv::IncomingRingtonePlayer player([&](const std::wstring& id,const std::atomic<bool>& cancel){
  const auto active=++concurrent;auto prior=maximum.load();while(active>prior && !maximum.compare_exchange_weak(prior,active)){}
  {std::lock_guard<std::mutex> lock(mutex);ids.push_back(id);}entered.notify_all();
  while(!cancel.load())std::this_thread::sleep_for(1ms);
  --concurrent;
  return axv::TonePreviewState::Failed; // Obsolete error must not overwrite replacement.
 },1s);
 auto wait=[&](std::size_t count){std::unique_lock<std::mutex> lock(mutex);CHECK(entered.wait_for(lock,2s,[&]{return ids.size()>=count;}));};
 CHECK(player.synchronize({1,1,L"first"}));wait(1);
 CHECK(player.synchronize({1,2,L"second"}));wait(2);
 auto snapshot=player.snapshot();CHECK(snapshot.request.session==1 && snapshot.request.call_id==2 && snapshot.result!=axv::TonePreviewState::Failed);
 CHECK(player.synchronize({1,2,L"changed-output"}));wait(3);
 {std::lock_guard<std::mutex> lock(mutex);CHECK(ids[2]==L"changed-output");}
 player.stop();CHECK(!player.snapshot().request.active());
 CHECK(player.synchronize({2,1,L""}));wait(4);
 {std::lock_guard<std::mutex> lock(mutex);CHECK(ids[3].empty());}
 CHECK(player.shutdown());CHECK(maximum==1 && concurrent==0);
 CHECK(!player.synchronize({3,1,L"third"}));
 std::atomic<int> attempts{0};
 axv::IncomingRingtonePlayer failing([&](const std::wstring&,const std::atomic<bool>&){++attempts;throw 1;return axv::TonePreviewState::Played;},1h);
 CHECK(failing.synchronize({5,1,L"missing"}));
 const auto limit=std::chrono::steady_clock::now()+2s;
 while(failing.snapshot().result!=axv::TonePreviewState::Failed && std::chrono::steady_clock::now()<limit)std::this_thread::yield();
 CHECK(failing.snapshot().result==axv::TonePreviewState::Failed && attempts==1);
 CHECK(!failing.synchronize({5,0,L"invalid"}));CHECK(!failing.snapshot().request.active());
 CHECK(failing.shutdown());
 std::atomic<int> retries{0};std::atomic<bool> release_retry{false};
 axv::IncomingRingtonePlayer retry([&](const std::wstring& id,const std::atomic<bool>& cancel){
  CHECK(id==L"same-explicit-output");
  if(++retries==1)return axv::TonePreviewState::MissingDevice;
  while(!release_retry && !cancel.load())std::this_thread::sleep_for(1ms);
  return axv::TonePreviewState::Played;
 },1ms);
 CHECK(retry.synchronize({8,1,L"same-explicit-output"}));
 const auto retry_limit=std::chrono::steady_clock::now()+2s;
 while(retries<2 && std::chrono::steady_clock::now()<retry_limit)std::this_thread::yield();
 CHECK(retries>=2 && retry.snapshot().result==axv::TonePreviewState::MissingDevice);
 release_retry=true;
 while(retry.snapshot().result!=axv::TonePreviewState::Played && std::chrono::steady_clock::now()<retry_limit)std::this_thread::yield();
 CHECK(retry.snapshot().result==axv::TonePreviewState::Played);
 CHECK(retry.shutdown());
 std::atomic<bool> rendering{false},finish{false};
 axv::IncomingRingtonePlayer draining([&](const std::wstring&,const std::atomic<bool>&){
  rendering=true;
  while(!finish)std::this_thread::sleep_for(1ms);
  return axv::TonePreviewState::Cancelled;
 });
 CHECK(draining.synchronize({9,1,L"output"}));
 const auto drain_limit=std::chrono::steady_clock::now()+2s;
 while(!rendering && std::chrono::steady_clock::now()<drain_limit)std::this_thread::yield();
 CHECK(rendering && draining.busy());
 draining.stop();CHECK(!draining.snapshot().request.active() && draining.busy());
 finish=true;CHECK(draining.shutdown());CHECK(!draining.busy());
}
