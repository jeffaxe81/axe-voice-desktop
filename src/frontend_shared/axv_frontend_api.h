#pragma once
// Versioned C ABI for future Win32 and WinUI 3 presentation adapters.
// NO SIP commands or credentials may cross this boundary in v1.
#include <stdint.h>

#if defined(_WIN32)
#  if defined(AXV_FRONTEND_BRIDGE_BUILD)
#    define AXV_FRONTEND_API __declspec(dllexport)
#  else
#    define AXV_FRONTEND_API __declspec(dllimport)
#  endif
#else
#  define AXV_FRONTEND_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AXV_FRONTEND_ABI_VERSION 1u
#define AXV_FRONTEND_OK 0
#define AXV_FRONTEND_ERROR_INVALID -1

// All fields are fixed-width integers; callers supply the expected exact size.
// State numbers mirror the existing C++ enum values and are validated.
typedef struct AxvFrontendObservation {
    uint32_t struct_size;
    uint32_t process;
    uint32_t sip;
    uint32_t evidence;
    uint64_t session;
    uint64_t call_id;
    uint64_t current_session;
    uint32_t dial_authorized;
} AxvFrontendObservation;

typedef struct AxvFrontendSnapshot {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t backend_attached; // 0 until the real native Baresip host is bound
    uint32_t connection;       // 0 offline, 1 pending, 2 verified online
    uint32_t view;             // 0 idle, 1 incoming, 2 outgoing, 3 connected
    uint32_t can_dial;
    uint32_t can_answer;
    uint32_t can_hangup;
    uint32_t can_secondary;
    uint64_t session;
    uint64_t call_id;
} AxvFrontendSnapshot;

AXV_FRONTEND_API uint32_t axv_frontend_abi_version(void);
// The v1 bridge is deliberately read-only and fail-closed. The WinUI 3
// candidate must NOT claim SIP support before a real native adapter exists.
AXV_FRONTEND_API int32_t axv_frontend_get_snapshot(
    AxvFrontendSnapshot* out, uint32_t out_size);
// Pure projection for ABI tests and later trusted native-producer integration.
// Does NOT attach a backend, authorize an action or originate SIP traffic.
AXV_FRONTEND_API int32_t axv_frontend_project_snapshot(
    const AxvFrontendObservation* observation, uint32_t observation_size,
    AxvFrontendSnapshot* out, uint32_t out_size);

#ifdef __cplusplus
}
#endif
