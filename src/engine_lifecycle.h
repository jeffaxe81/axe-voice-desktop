#pragma once
namespace axv {
enum class EngineLifecycle { Stopped, Starting, Running, Stopping, Failed };
enum class LifecycleEvent { StartRequested, Started, StartFailed, StopRequested, Stopped, Fault };
inline EngineLifecycle transition(EngineLifecycle state, LifecycleEvent event) {
    switch (state) {
    case EngineLifecycle::Stopped:
        if (event == LifecycleEvent::StartRequested) return EngineLifecycle::Starting;
        break;
    case EngineLifecycle::Starting:
        if (event == LifecycleEvent::Started) return EngineLifecycle::Running;
        if (event == LifecycleEvent::StartFailed || event == LifecycleEvent::Fault) return EngineLifecycle::Failed;
        if (event == LifecycleEvent::StopRequested) return EngineLifecycle::Stopping;
        break;
    case EngineLifecycle::Running:
        if (event == LifecycleEvent::StopRequested) return EngineLifecycle::Stopping;
        if (event == LifecycleEvent::Fault) return EngineLifecycle::Failed;
        break;
    case EngineLifecycle::Stopping:
        if (event == LifecycleEvent::Stopped) return EngineLifecycle::Stopped;
        if (event == LifecycleEvent::Fault) return EngineLifecycle::Failed;
        break;
    case EngineLifecycle::Failed:
        if (event == LifecycleEvent::StopRequested) return EngineLifecycle::Stopping;
        break;
    }
    return state;
}
}
