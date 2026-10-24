#include "baresip_sdk_abi_contract.h"
// This executable is intentionally NEVER run by default. Its job is to force
// the Windows linker to resolve symbols from the audited import libraries.
// Volatile function pointers generate actual link relocations, unlike mere
// sizeof/decltype checks. No SIP/network operation occurs at runtime.
decltype(&bevent_register) volatile axv_link_event = &bevent_register;
decltype(&ua_register) volatile axv_link_registration = &ua_register;
decltype(&ua_isregistered) volatile axv_link_status = &ua_isregistered;
decltype(&ua_alloc) volatile axv_link_account = &ua_alloc;
decltype(&ua_destroy) volatile axv_link_account_destroy = &ua_destroy;
decltype(&libre_init) volatile axv_link_libre = &libre_init;
decltype(&re_main) volatile axv_link_loop = &re_main;
decltype(&mqueue_alloc) volatile axv_link_queue = &mqueue_alloc;
decltype(&mqueue_push) volatile axv_link_push = &mqueue_push;

int main() {
    return axv_link_event && axv_link_registration && axv_link_status &&
        axv_link_account && axv_link_account_destroy && axv_link_libre &&
        axv_link_loop && axv_link_queue && axv_link_push ? 0 : 1;
}
