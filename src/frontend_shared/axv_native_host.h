#pragma once
// Internal C++ host API, never exposed as a C++ ABI to managed WinUI 3.
#include "axv_frontend_api.h"
#include <string>
#include <cstdint>

namespace axv {
int32_t native_host_get_snapshot(AxvFrontendSnapshot* out) noexcept;
int32_t native_host_connect_saved() noexcept;
int32_t native_host_disconnect() noexcept;
int32_t native_host_dial(const std::wstring& destination,
                         std::uint64_t expected_session) noexcept;
int32_t native_host_answer(std::uint64_t expected_session,
                           std::uint64_t expected_call_id) noexcept;
int32_t native_host_hangup(std::uint64_t expected_session,
                           std::uint64_t expected_call_id) noexcept;
}
