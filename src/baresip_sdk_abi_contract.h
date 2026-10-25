#pragma once
// Compile-time type contract against audited Baresip and libre headers.
// These declarations intentionally mirror the upstream public API; no SDK
// calls occur here. SHA preflight separately pins local SDK bytes.
#include <re.h>
#include <baresip.h>
#include <string_view>
#include <type_traits>

namespace axv {
static_assert(std::is_same_v<decltype(&bevent_register),
              int (*)(bevent_h*, void*)>,
              "Baresip bevent_register ABI changed");
static_assert(std::is_same_v<decltype(&bevent_unregister),
              void (*)(bevent_h*)>,
              "Baresip bevent_unregister ABI changed");
static_assert(std::is_same_v<decltype(&bevent_get_ua),
              struct ua* (*)(const struct bevent*)>,
              "Baresip bevent_get_ua ABI changed");
static_assert(std::is_same_v<decltype(&bevent_get_call),
              struct call* (*)(const struct bevent*)>,
              "Baresip bevent_get_call ABI changed");
static_assert(std::is_same_v<decltype(&ua_alloc),
              int (*)(struct ua**, const char*)>,
              "Baresip ua_alloc ABI changed");
static_assert(std::is_same_v<decltype(&ua_destroy),
              unsigned (*)(struct ua*)>,
              "Baresip ua_destroy ABI changed");
static_assert(std::is_same_v<decltype(&ua_register),
              int (*)(struct ua*)>,
              "Baresip ua_register ABI changed");
static_assert(std::is_same_v<decltype(&ua_unregister),
              void (*)(struct ua*)>,
              "Baresip ua_unregister ABI changed");
static_assert(std::is_same_v<decltype(&ua_isregistered),
              bool (*)(const struct ua*)>,
              "Baresip ua_isregistered ABI changed");
static_assert(std::is_same_v<decltype(&libre_init), int (*)()>,
              "libre libre_init ABI changed");
static_assert(std::is_same_v<decltype(&libre_close), void (*)()>,
              "libre libre_close ABI changed");
static_assert(std::is_same_v<decltype(&re_main), int (*)(re_signal_h*)>,
              "libre re_main ABI changed");
static_assert(std::is_same_v<decltype(&re_cancel), void (*)()>,
              "libre re_cancel ABI changed");
static_assert(std::is_same_v<decltype(&mqueue_alloc),
              int (*)(struct mqueue**, mqueue_h*, void*)>,
              "libre mqueue_alloc ABI changed");
static_assert(std::is_same_v<decltype(&mqueue_push),
              int (*)(struct mqueue*, int, void*)>,
              "libre mqueue_push ABI changed");
#ifdef AXV_BARESIP_AUDITED_VERSION
static_assert(std::string_view(BARESIP_VERSION) ==
              std::string_view(AXV_BARESIP_AUDITED_VERSION),
              "Baresip audited version differs from compiled header");
#endif
} // namespace axv
