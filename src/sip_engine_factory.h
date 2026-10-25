#pragma once
#include "sip_engine.h"
#include <memory>
#ifdef AXV_ENABLE_NATIVE_SIP
#include "baresip_sip_engine.h"
#endif

namespace axv {
enum class EngineKind { Unavailable, Baresip };
struct EngineSelection {
    std::unique_ptr<SipEngine> engine;
    EngineKind kind;
};

// Fail closed until Baresip runtime is implemented, linked and tested.
// No simulator reports successful registration or calls.
inline EngineSelection create_sip_engine() {
#ifdef AXV_ENABLE_NATIVE_SIP
    return {std::make_unique<BaresipSipEngine>(), EngineKind::Baresip};
#else
    return {std::make_unique<UnavailableSipEngine>(), EngineKind::Unavailable};
#endif
}
}
