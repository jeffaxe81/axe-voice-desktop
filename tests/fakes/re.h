#pragma once
// Intentionally tiny TEST-ONLY declarations; never install as libre/re SDK.
#ifdef __cplusplus
extern "C" {
#endif
struct mqueue;
typedef void (mqueue_h)(int id, void* data, void* arg);
typedef void (re_signal_h)(int sig);
int libre_init(void);
void libre_close(void);
int re_main(re_signal_h* signalh);
void re_cancel(void);
int mqueue_alloc(struct mqueue** mqp, mqueue_h* h, void* arg);
int mqueue_push(struct mqueue* mq, int id, void* data);
#ifdef __cplusplus
}
#endif
