#include "../src/sip_engine_factory.h"
int main() {
    auto selection = axv::create_sip_engine();
    if (!selection.engine) return 1;
    if (selection.kind != axv::EngineKind::Unavailable) return 2;
    if (selection.engine->status() != axv::SipStatus::Unavailable) return 3;
    if (selection.engine->register_account()) return 4;
    if (selection.engine->start_call(L"sip:1002@example.org")) return 5;
    return 0;
}
