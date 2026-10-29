#include "../src/incoming_ringtone_player.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <new>
#include <thread>

namespace {
const auto main_thread = std::this_thread::get_id();
std::atomic<bool> reject_worker_allocations{false};
std::atomic<unsigned> rejected_allocations{0};
}

void* operator new(std::size_t size) {
 if (std::this_thread::get_id() != main_thread && reject_worker_allocations.load()) {
  ++rejected_allocations;
  throw std::bad_alloc();
 }
 if (void* pointer = std::malloc(size ? size : 1)) return pointer;
 throw std::bad_alloc();
}
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

#define CHECK(value) do { if (!(value)) { \
 std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #value); \
 std::abort(); } } while (false)

template<class Predicate> void await(Predicate predicate) {
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
 while (!predicate() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
 CHECK(predicate());
}

int main() {
 using namespace std::chrono_literals;
 // A regression must fail visibly, without invoking platform crash dialogs.
 std::set_terminate([] {
  std::fputs("Unexpected termination after worker allocation failure\n", stderr);
  std::_Exit(77);
 });
 const axv::IncomingRingtoneRequest request{7, 11, std::wstring(2048, L'x')};
 std::atomic<unsigned> renders{0};
 axv::IncomingRingtonePlayer player([&](const std::wstring& endpoint, const std::atomic<bool>& cancel) {
  CHECK(endpoint == request.endpoint_id);
  ++renders;
  while (!cancel.load()) std::this_thread::yield();
  return axv::TonePreviewState::Cancelled;
 }, 1h);
 // Caller-side request/thread allocations remain available; only the real
 // worker's endpoint snapshot copy fails, before entering the renderer.
 reject_worker_allocations = true;
 CHECK(player.synchronize(request));
 await([&] { return player.snapshot().result == axv::TonePreviewState::Failed; });
 CHECK(rejected_allocations == 1 && renders == 0);
 const auto failed = player.snapshot();
 CHECK(failed.request.session == 7 && failed.request.call_id == 11);
 CHECK(failed.request.endpoint_id == request.endpoint_id);
 CHECK(player.synchronize(request));
 CHECK(player.snapshot().result == axv::TonePreviewState::Failed && renders == 0);

 // Stop changes the generation and leaves no canceled render running. The
 // identical call can then be installed again and rendered by the same worker.
 player.stop();
 CHECK(!player.busy() && !player.snapshot().request.active());
 reject_worker_allocations = false;
 CHECK(player.synchronize(request));
 await([&] { return renders == 1; });
 CHECK(player.snapshot().result == axv::TonePreviewState::Playing);
 CHECK(player.shutdown());
 CHECK(!player.busy() && !player.snapshot().request.active());
 CHECK(!player.synchronize(request));

 // A transient snapshot failure also recovers on the normal retry interval,
 // without requiring a new call or clearing the error before a render succeeds.
 std::atomic<bool> render_entered{false},finish_render{false};
 axv::IncomingRingtonePlayer retry([&](const std::wstring& endpoint, const std::atomic<bool>& cancel) {
  CHECK(endpoint == request.endpoint_id);
  render_entered = true;
  while (!finish_render && !cancel.load()) std::this_thread::yield();
  return axv::TonePreviewState::Played;
 }, 1ms);
 reject_worker_allocations = true;
 CHECK(retry.synchronize(request));
 await([&] { return retry.snapshot().result == axv::TonePreviewState::Failed; });
 CHECK(!render_entered);
 reject_worker_allocations = false;
 await([&] { return render_entered.load(); });
 CHECK(retry.snapshot().result == axv::TonePreviewState::Failed);
 finish_render = true;
 await([&] { return retry.snapshot().result == axv::TonePreviewState::Played; });
 CHECK(retry.shutdown());
}
