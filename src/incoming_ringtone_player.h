#pragma once
#include "incoming_ringtone_policy.h"
#include "audio_test_tone_win32.h"
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
namespace axv {
struct IncomingRingtoneSnapshot {IncomingRingtoneRequest request;TonePreviewState result=TonePreviewState::Idle;};
// Render is a cooperative render-only platform port. No UI/native SIP pointers.
// Call shutdown/destruction outside Render; caller serializes destruction.
class IncomingRingtonePlayer {
public:
 using Render=std::function<TonePreviewState(const std::wstring&,const std::atomic<bool>&)>;
 explicit IncomingRingtonePlayer(Render render,std::chrono::milliseconds gap=std::chrono::milliseconds(1250));
 ~IncomingRingtonePlayer();
 IncomingRingtonePlayer(const IncomingRingtonePlayer&)=delete;
 IncomingRingtonePlayer& operator=(const IncomingRingtonePlayer&)=delete;
 bool synchronize(const IncomingRingtoneRequest& request);
 void stop(); // Nonblocking cancellation request; renderer cooperates.
 bool shutdown();
 IncomingRingtoneSnapshot snapshot()const;
private:
 void run()noexcept;
 Render render_;
 std::chrono::milliseconds gap_;
 mutable std::mutex mutex_;
 std::condition_variable changed_;
 IncomingRingtoneSnapshot state_;
 std::uint64_t generation_=0;
 bool closed_=false;
 std::atomic<bool> cancel_{true};
 std::thread worker_;
};
}
