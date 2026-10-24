#include "../src/engine_lifecycle.h"
int main() {
    using namespace axv;
    auto state = EngineLifecycle::Stopped;
    state = transition(state, LifecycleEvent::StartRequested);
    if (state != EngineLifecycle::Starting) return 1;
    state = transition(state, LifecycleEvent::Started);
    if (state != EngineLifecycle::Running) return 2;
    state = transition(state, LifecycleEvent::Fault);
    if (state != EngineLifecycle::Failed) return 3;
    state = transition(state, LifecycleEvent::StartRequested);
    if (state != EngineLifecycle::Failed) return 4;
    state = transition(state, LifecycleEvent::StopRequested);
    if (state != EngineLifecycle::Stopping) return 5;
    state = transition(state, LifecycleEvent::Stopped);
    if (state != EngineLifecycle::Stopped) return 6;
    return 0;
}
