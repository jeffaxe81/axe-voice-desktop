#pragma once
#define BARESIP_VERSION "4.12.0"
// Test-double SDK subset only. Never install or ship as a Baresip SDK header.
#ifdef __cplusplus
extern "C" {
#endif
struct ua { unsigned test_id; };
struct call { struct ua* owner; unsigned test_id; };
struct bevent { struct ua* owner; struct call* active_call; };
enum bevent_ev {
  BEVENT_REGISTERING, BEVENT_REGISTER_OK, BEVENT_REGISTER_FAIL,
  BEVENT_UNREGISTERING, BEVENT_SHUTDOWN, BEVENT_EXIT,
  BEVENT_CALL_INCOMING, BEVENT_CALL_OUTGOING,
  BEVENT_CALL_ESTABLISHED, BEVENT_CALL_CLOSED,
  BEVENT_CALL_PROGRESS, BEVENT_AUDIO_ERROR, BEVENT_SIPSESS_FAILED,
  BEVENT_SIPSESS_CONN, BEVENT_CALL_RTPESTAB, BEVENT_CALL_MENC
};
typedef void (bevent_h)(enum bevent_ev, struct bevent*, void*);
int bevent_register(bevent_h* handler, void* argument);
void bevent_unregister(bevent_h* handler);
struct ua* bevent_get_ua(const struct bevent* event);
struct call* bevent_get_call(const struct bevent* event);
enum vidmode { VIDMODE_OFF = 0, VIDMODE_ON = 1 };
int ua_connect(struct ua* ua, struct call** callp, const char* from_uri,
               const char* req_uri, enum vidmode vmode);
int ua_answer(struct ua* ua, struct call* call, enum vidmode vmode);
void ua_hangup(struct ua* ua, struct call* call, unsigned short scode,
               const char* reason);
int ua_alloc(struct ua** selected, const char* aor);
unsigned ua_destroy(struct ua* selected);
int ua_register(struct ua* selected);
void ua_unregister(struct ua* selected);
bool ua_isregistered(const struct ua* selected);
#ifdef __cplusplus
}
#endif
