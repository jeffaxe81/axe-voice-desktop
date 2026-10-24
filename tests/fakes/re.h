#pragma once
// Intentionally tiny TEST-ONLY declarations; never install as libre/re SDK.
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
struct mqueue;
typedef void (tmr_h)(void* arg);
struct tmr { tmr_h* handler; void* arg; uint64_t wait_ms; };
typedef void (mqueue_h)(int id, void* data, void* arg);
typedef void (re_signal_h)(int sig);
int libre_init(void);
void libre_close(void);
int re_main(re_signal_h* signalh);
void re_cancel(void);
int mqueue_alloc(struct mqueue** mqp, mqueue_h* h, void* arg);
int mqueue_push(struct mqueue* mq, int id, void* data);
void* mem_deref(void* data);
void tmr_init(struct tmr* t);
void tmr_start_dbg(struct tmr* t, uint64_t delay, tmr_h* h, void* arg,
                   const char* file, int line);
void tmr_cancel(struct tmr* t);
#define tmr_start(t,delay,handler,arg) \
    tmr_start_dbg((t),(delay),(handler),(arg),__FILE__,__LINE__)
#ifdef __cplusplus
}
#endif
