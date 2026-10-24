#pragma once
#include "sip_engine.h"
#include <memory>

namespace axv {
enum class EngineKind { Unavailable, Baresip };
struct EngineSelection {
    std::unique_ptr<SipEngine> engine;
    EngineKind kind;
};

// Fail closed until Baresip runtime is implemented, linked and tested.
// No simulator reports successful registration or calls.
inline EngineSelection create_sip_engine() {
    return {std::make_unique<UnavailableSipEngine>(), EngineKind::Unavailable};
}
}
