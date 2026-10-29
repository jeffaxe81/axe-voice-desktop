#include "incoming_ringtone_player.h"
#include <limits>
#include <utility>
namespace axv {
IncomingRingtonePlayer::IncomingRingtonePlayer(Render render,std::chrono::milliseconds gap)
 :render_(std::move(render)),gap_(gap.count()>0?gap:std::chrono::milliseconds(1250)){}
IncomingRingtonePlayer::~IncomingRingtonePlayer(){(void)shutdown();}
bool IncomingRingtonePlayer::synchronize(const IncomingRingtoneRequest& input){
 const bool invalid=(bool(input.session)!=bool(input.call_id)) || !valid_ringtone_endpoint(input.endpoint_id);
 const auto next=invalid || !input.active()?IncomingRingtoneRequest{}:input;
 std::lock_guard<std::mutex> lock(mutex_);
 if(closed_)return false;
 if(state_.request.session==next.session && state_.request.call_id==next.call_id && state_.request.endpoint_id==next.endpoint_id)return !invalid;
 cancel_.store(true);
 if(generation_==std::numeric_limits<std::uint64_t>::max()){
  closed_=true;state_={{},TonePreviewState::Failed};changed_.notify_all();return false;
 }
 ++generation_;
 state_={next,next.active()?TonePreviewState::Playing:TonePreviewState::Idle};
 if(next.active() && !worker_.joinable()){
  try{worker_=std::thread([this]{run();});}
  catch(...){state_={{},TonePreviewState::Failed};return false;}
 }
 changed_.notify_all();return !invalid;
}
void IncomingRingtonePlayer::stop(){(void)synchronize({});}
bool IncomingRingtonePlayer::shutdown(){
 {std::lock_guard<std::mutex> lock(mutex_);closed_=true;cancel_.store(true);state_={};}
 changed_.notify_all();
 if(worker_.joinable()){
  if(worker_.get_id()==std::this_thread::get_id())return false;
  worker_.join();
 }
 return true;
}
IncomingRingtoneSnapshot IncomingRingtonePlayer::snapshot()const{std::lock_guard<std::mutex> lock(mutex_);return state_;}
bool IncomingRingtonePlayer::busy()const{std::lock_guard<std::mutex> lock(mutex_);return state_.request.active() || rendering_;}
void IncomingRingtonePlayer::run()noexcept{
 std::unique_lock<std::mutex> lock(mutex_);
 while(!closed_){
  changed_.wait(lock,[this]{return closed_ || state_.request.active();});
  if(closed_)break;
  const auto generation=generation_;
  IncomingRingtoneRequest request;
  try{request=state_.request;}
  catch(...){
   // Keep the call identity so the UI can report this failed attempt. Retry
   // with the usual gap; cancellation or a replacement generation wakes us.
   state_.result=TonePreviewState::Failed;
   changed_.wait_for(lock,gap_,[this,generation]{return closed_ || generation!=generation_;});
   continue;
  }
  cancel_.store(false);rendering_=true;
  lock.unlock();
  TonePreviewState result=TonePreviewState::Failed;
  try{result=render_(request.endpoint_id,cancel_);}catch(...){result=TonePreviewState::Failed;}
  lock.lock();rendering_=false;
  if(generation==generation_ && !closed_)state_.result=result;
  changed_.wait_for(lock,gap_,[this,generation]{return closed_ || generation!=generation_;});
 }
}
}
