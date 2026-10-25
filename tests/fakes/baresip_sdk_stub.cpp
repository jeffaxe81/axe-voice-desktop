#include <baresip.h>
static bevent_h* g_handler = nullptr;
extern "C" int bevent_register(bevent_h* handler, void*) {
    if (!handler || g_handler) return 22;
    g_handler = handler;
    return 0;
}
extern "C" void bevent_unregister(bevent_h* handler) {
    if (g_handler == handler) g_handler = nullptr;
}
extern "C" struct ua* bevent_get_ua(const struct bevent* event) {
    return event ? event->owner : nullptr;
}
extern "C" struct call* bevent_get_call(const struct bevent* event) {
    return event ? event->active_call : nullptr;
}

extern "C" int ua_connect(struct ua*, struct call**, const char*,
                          const char*, enum vidmode) { return 95; }
extern "C" int ua_answer(struct ua*, struct call*, enum vidmode) {
    return 95;
}
extern "C" void ua_hangup(struct ua*, struct call*, unsigned short,
                          const char*) {}

// Synthetic registrar confirmation for this focused ownership/call policy fixture.
extern "C" bool ua_isregistered(const struct ua* selected) { return selected != nullptr; }
