#pragma once
// Optional libre control-plane reactor: NOT a Baresip SIP/account engine.
// The real SDK remains opt-in behind audited headers/libraries.
#include <re.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

namespace axv {
enum class LibreReactorFault {
    None, InitRejected, QueueRejected, HookFailed, MainLoopFailed
};
inline const wchar_t* libre_reactor_fault_message(LibreReactorFault fault) {
    switch(fault) {
    case LibreReactorFault::InitRejected:return L"Falha ao iniciar motor nativo; senha descartada. Inicializacao libre recusada.";
    case LibreReactorFault::QueueRejected:return L"Falha ao iniciar motor nativo; senha descartada. Fila nativa indisponivel.";
    case LibreReactorFault::HookFailed:return L"Falha em etapa do motor nativo; sessao encerrada e senha descartada. Pare o motor e registre novamente.";
    case LibreReactorFault::MainLoopFailed:return L"Falha no ciclo do motor nativo; sessao encerrada e senha descartada. Pare o motor e registre novamente.";
    case LibreReactorFault::None:return L"";
    }
    return L"Falha do motor nativo. Pare o motor antes de tentar novamente.";
}
struct LibreReactorSnapshot {
    bool running=false;
    std::uint64_t generation=0;
    std::uint64_t ticks=0;
    LibreReactorFault fault=LibreReactorFault::None;
};

// Owns a libre re_main() thread and a native mqueue in that SAME thread.
// on_start/on_tick/on_stop run exclusively on that thread. Only wake()/stop()
// can be called by clients; they never touch Baresip UA or credentials.
// The stopping hook must tolerate partial startup; it runs once after the start
// hook is attempted, including when that hook throws. No hooks run if init/queue fails.
// Caller must serialize start(), stop() and destruction from a non-owner thread.
class BaresipNativeLibreReactor {
public:
    using Hook=std::function<void()>;
    explicit BaresipNativeLibreReactor(
        Hook tick, Hook started={}, Hook stopping={},
        std::chrono::milliseconds cadence=std::chrono::milliseconds(20));
    BaresipNativeLibreReactor(const BaresipNativeLibreReactor&)=delete;
    BaresipNativeLibreReactor& operator=(const BaresipNativeLibreReactor&)=delete;
    ~BaresipNativeLibreReactor();

    bool start();
    bool wake();
    bool stop();
    LibreReactorSnapshot snapshot() const;

private:
    static constexpr int kWake=1;
    static constexpr int kStop=2;
    static void queue_handler(int id, void* data, void* arg) noexcept;
    static void timer_handler(void* arg) noexcept;
    void tick_on_owner() noexcept;
    void run() noexcept;
    void publish_ready(bool running, LibreReactorFault fault);
    Hook tick_,started_,stopping_;
    const std::chrono::milliseconds cadence_;
    mutable std::mutex mutex_;
    std::condition_variable ready_cv_;
    struct mqueue* queue_=nullptr;
    struct tmr timer_{};
    std::thread worker_;
    LibreReactorSnapshot status_{};
    bool ready_=false;
    std::atomic<bool> ending_{true};
};
} // namespace axv
