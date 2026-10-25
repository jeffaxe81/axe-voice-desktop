#include "../src/incoming_ringtone_player.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
int main(){
 using namespace std::chrono_literals;
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
}
