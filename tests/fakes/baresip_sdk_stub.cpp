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

extern "C" struct audio* call_audio(const struct call* selected){return selected?&const_cast<struct call*>(selected)->test_audio:nullptr;}
extern "C" void audio_mute(struct audio* selected,bool muted){if(selected)selected->test_muted=muted;}
extern "C" bool audio_ismuted(const struct audio* selected){return selected && selected->test_muted;}
extern "C" int call_hold(struct call* selected,bool held){if(!selected)return 22;selected->test_held=held;return 0;}
extern "C" bool call_is_onhold(const struct call* selected){return selected && selected->test_held;}

extern "C" int call_transfer(struct call*,const char*){return 95;}

extern "C" int call_send_digit(struct call*,char){return 95;}
extern "C" struct account* call_account(const struct call*){return nullptr;}
extern "C" enum dtmfmode account_dtmfmode(const struct account*){return DTMFMODE_RTP_EVENT;}
