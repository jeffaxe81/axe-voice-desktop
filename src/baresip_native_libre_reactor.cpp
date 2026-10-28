#include "baresip_native_libre_reactor.h"
#include <limits>
#include <utility>

namespace axv {
BaresipNativeLibreReactor::BaresipNativeLibreReactor(
    Hook tick,Hook started,Hook stopping,std::chrono::milliseconds cadence)
  : tick_(std::move(tick)),started_(std::move(started)),
    stopping_(std::move(stopping)),
    cadence_(cadence.count()>0?cadence:std::chrono::milliseconds(20)) {}

BaresipNativeLibreReactor::~BaresipNativeLibreReactor() { (void)stop(); }

bool BaresipNativeLibreReactor::start() {
    if (worker_.joinable()) return false;
    ending_=false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ready_=false;
        status_.running=false;
        status_.ticks=0;
        status_.fault=LibreReactorFault::None;
    }
    worker_=std::thread([this] { run(); });
    std::unique_lock<std::mutex> lock(mutex_);
    ready_cv_.wait(lock,[this]{return ready_;});
    return status_.running;
}

bool BaresipNativeLibreReactor::wake() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ending_ || !queue_ || !status_.running) return false;
    return mqueue_push(queue_,kWake,nullptr)==0;
}

bool BaresipNativeLibreReactor::stop() {
    if (!worker_.joinable()) return true;
    if (worker_.get_id()==std::this_thread::get_id()) return false;
    ending_=true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // If mqueue_push fails, the periodic native timer observes ending_
        // and calls re_cancel() on the owning re_main() thread.
        if (queue_) (void)mqueue_push(queue_,kStop,nullptr);
    }
    worker_.join();
    return true;
}

LibreReactorSnapshot BaresipNativeLibreReactor::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

void BaresipNativeLibreReactor::publish_ready(bool running,LibreReactorFault fault) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ready_=true;
        status_.running=running;
        status_.fault=fault;
        if (running && status_.generation!=std::numeric_limits<std::uint64_t>::max())
            ++status_.generation;
    }
    ready_cv_.notify_all();
}

void BaresipNativeLibreReactor::tick_on_owner() noexcept {
    if (ending_) { re_cancel(); return; }
    try {
        if (tick_) tick_();
        std::lock_guard<std::mutex> lock(mutex_);
        ++status_.ticks;
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(status_.fault==LibreReactorFault::None)status_.fault=LibreReactorFault::HookFailed;
        }
        ending_=true;
        re_cancel();
    }
}

void BaresipNativeLibreReactor::queue_handler(int id,void*,void* arg) noexcept {
    auto* self=static_cast<BaresipNativeLibreReactor*>(arg);
    if (!self) return;
    if (id==kStop || self->ending_) { re_cancel(); return; }
    if (id==kWake) self->tick_on_owner();
}

void BaresipNativeLibreReactor::timer_handler(void* arg) noexcept {
    auto* self=static_cast<BaresipNativeLibreReactor*>(arg);
    if (!self) return;
    if (self->ending_) { re_cancel(); return; }
    self->tick_on_owner();
    if (self->ending_) { re_cancel(); return; }
    tmr_start(&self->timer_,
              static_cast<std::uint64_t>(self->cadence_.count()),
              &BaresipNativeLibreReactor::timer_handler,self);
}

void BaresipNativeLibreReactor::run() noexcept {
    // All libre allocations and callbacks remain on the owner thread.
    const int initial=libre_init();
    if (initial) {
        publish_ready(false,LibreReactorFault::InitRejected);
        return;
    }
    struct mqueue* allocated=nullptr;
    const int result=mqueue_alloc(&allocated,
                                 &BaresipNativeLibreReactor::queue_handler,this);
    if (result || !allocated) {
        publish_ready(false,LibreReactorFault::QueueRejected);
        libre_close();
        return;
    }
    tmr_init(&timer_);
    bool began=false;
    try {
        if (started_) started_();
        began=true;
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(status_.fault==LibreReactorFault::None)status_.fault=LibreReactorFault::HookFailed;
        }
    }
    if (!began) {
        publish_ready(false,LibreReactorFault::HookFailed);
    } else {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_=allocated;
        }
        publish_ready(true,LibreReactorFault::None);
        tmr_start(&timer_,static_cast<std::uint64_t>(cadence_.count()),
                  &BaresipNativeLibreReactor::timer_handler,this);
        const int loop_error=re_main(nullptr);
        if (loop_error) {
            std::lock_guard<std::mutex> lock(mutex_);
            if(status_.fault==LibreReactorFault::None)status_.fault=LibreReactorFault::MainLoopFailed;
        }
    }
    ending_=true;
    tmr_cancel(&timer_);
    try { if (stopping_) stopping_(); }
    catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(status_.fault==LibreReactorFault::None)status_.fault=LibreReactorFault::HookFailed;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_=nullptr; // Reject foreign wake before mem_deref.
        status_.running=false;
    }
    (void)mem_deref(allocated);
    libre_close();
    ready_cv_.notify_all();
}
} // namespace axv
