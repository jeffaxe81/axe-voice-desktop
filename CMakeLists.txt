cmake_minimum_required(VERSION 3.20)
project(AXEVoiceDesktop VERSION 0.42.0 LANGUAGES CXX)
if(NOT WIN32)
  message(FATAL_ERROR "This bootstrap currently supports Windows only")
endif()
add_library(axv_sip_address STATIC src/sip_address.cpp)
target_compile_features(axv_sip_address PUBLIC cxx_std_17)
add_library(axv_audio_devices STATIC src/audio_devices_win32.cpp)
target_compile_features(axv_audio_devices PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(axv_audio_devices PRIVATE /utf-8 /W4)
endif()
target_link_libraries(axv_audio_devices PUBLIC ole32 uuid propsys)
add_library(axv_windows_process STATIC src/windows_process.cpp)
target_compile_features(axv_windows_process PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(axv_windows_process PRIVATE /utf-8 /W4)
endif()
option(AXV_BUILD_NATIVE_BARESIP_ADAPTER
  "Compile isolated Baresip C API callback adapter (NOT linked into AXEVoice)" OFF)
if(AXV_BUILD_NATIVE_BARESIP_ADAPTER)
  set(AXV_BARESIP_INCLUDE_DIR "" CACHE PATH "Audited Baresip SDK include path")
  set(AXV_BARESIP_LIBRARY "" CACHE FILEPATH "Audited Baresip SDK MSVC import library")
  set(AXV_BARESIP_HEADER_SHA256 "" CACHE STRING "Audited baresip.h SHA-256")
  set(AXV_BARESIP_LIBRARY_SHA256 "" CACHE STRING "Audited Baresip .lib SHA-256")
  set(AXV_BARESIP_VERSION "" CACHE STRING "Audited BARESIP_VERSION macro value")
  set(AXV_LIBRE_INCLUDE_DIR "" CACHE PATH "Audited libre/re include path")
  set(AXV_LIBRE_LIBRARY "" CACHE FILEPATH "Audited libre/re MSVC import library")
  set(AXV_LIBRE_HEADER_SHA256 "" CACHE STRING "Audited re.h SHA-256")
  set(AXV_LIBRE_LIBRARY_SHA256 "" CACHE STRING "Audited libre/re .lib SHA-256")
  set(AXV_REPOSITORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/axv_verify_baresip_sdk.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/axv_verify_libre_sdk.cmake")

  # SDK remains entirely optional and is NEVER linked into AXEVoice GUI.
  add_library(axv_native_baresip_adapter STATIC src/baresip_sdk_event_adapter.cpp)
  target_include_directories(axv_native_baresip_adapter PRIVATE
    "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
  target_link_libraries(axv_native_baresip_adapter PUBLIC
    "${AXV_BARESIP_LIBRARY}" "${AXV_LIBRE_LIBRARY}")
  target_compile_features(axv_native_baresip_adapter PRIVATE cxx_std_17)

  # Unlike a static library alone, an EXE link probe must resolve the actual
  # SDK symbols. Builds fail on ABI/header/import-library mismatches.
  add_executable(axv_native_baresip_abi_probe src/baresip_sdk_abi_probe.cpp)
  target_include_directories(axv_native_baresip_abi_probe PRIVATE
    "${AXV_LIBRE_INCLUDE_DIR}" "${AXV_BARESIP_INCLUDE_DIR}")
  target_compile_definitions(axv_native_baresip_abi_probe PRIVATE
    AXV_BARESIP_AUDITED_VERSION="${AXV_BARESIP_VERSION}")
  target_link_libraries(axv_native_baresip_abi_probe PRIVATE
    "${AXV_BARESIP_LIBRARY}" "${AXV_LIBRE_LIBRARY}")
  target_compile_features(axv_native_baresip_abi_probe PRIVATE cxx_std_17)
  # The SDK-enabled test exercises the REAL libre re_main/mqueue/timer
  # without creating a Baresip UA, loading account secrets or sending SIP.
  add_executable(axv_native_libre_reactor_smoke
    tests/native_libre_reactor_smoke.cpp
    src/baresip_native_libre_reactor.cpp)
  target_include_directories(axv_native_libre_reactor_smoke PRIVATE
    "${AXV_LIBRE_INCLUDE_DIR}")
  target_link_libraries(axv_native_libre_reactor_smoke PRIVATE
    "${AXV_LIBRE_LIBRARY}")
  target_compile_features(axv_native_libre_reactor_smoke PRIVATE cxx_std_17)
endif()
add_executable(AXEVoice WIN32 src/main.cpp)
target_compile_features(AXEVoice PRIVATE cxx_std_17)
target_compile_definitions(AXEVoice PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
if(MSVC)
  target_compile_options(AXEVoice PRIVATE /utf-8 /W4)
endif()
target_link_libraries(AXEVoice PRIVATE user32 gdi32 advapi32 shell32 axv_sip_address axv_audio_devices axv_windows_process)
include(CTest)
if(BUILD_TESTING)
  if(AXV_BUILD_NATIVE_BARESIP_ADAPTER)
    add_test(NAME native_libre_reactor_smoke
      COMMAND axv_native_libre_reactor_smoke)
    set_tests_properties(native_libre_reactor_smoke PROPERTIES TIMEOUT 15)
  endif()
  add_executable(test_libre_owner_reactor
    tests/test_libre_owner_reactor.cpp
    src/baresip_native_libre_reactor.cpp)
  target_include_directories(test_libre_owner_reactor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_libre_owner_reactor PRIVATE cxx_std_17)
  add_test(NAME libre_owner_reactor COMMAND test_libre_owner_reactor)
  add_executable(test_libre_registration_owner_integration
    tests/test_libre_registration_owner_integration.cpp
    src/baresip_native_libre_reactor.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_libre_registration_owner_integration PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_libre_registration_owner_integration PRIVATE cxx_std_17)
  add_test(NAME libre_registration_owner_integration
    COMMAND test_libre_registration_owner_integration)
  set_tests_properties(libre_registration_owner_integration PROPERTIES TIMEOUT 15)
  set_tests_properties(libre_owner_reactor PROPERTIES TIMEOUT 15)
  add_executable(test_ui_dpi_policy tests/test_ui_dpi_policy.cpp)
  target_compile_features(test_ui_dpi_policy PRIVATE cxx_std_17)
  add_test(NAME ui_dpi_policy COMMAND test_ui_dpi_policy)
  add_executable(test_ui_contrast_policy tests/test_ui_contrast_policy.cpp)
  target_compile_features(test_ui_contrast_policy PRIVATE cxx_std_17)
  add_test(NAME ui_contrast_policy COMMAND test_ui_contrast_policy)
  add_test(NAME ui_smoke COMMAND $<TARGET_FILE:AXEVoice> --ui-smoke)
  add_executable(test_call_readiness tests/test_call_readiness.cpp)
  target_compile_features(test_call_readiness PRIVATE cxx_std_17)
  add_test(NAME call_readiness COMMAND test_call_readiness)
  set_tests_properties(ui_smoke PROPERTIES TIMEOUT 10)
  add_executable(test_dialpad_model tests/test_dialpad_model.cpp)
  target_compile_features(test_dialpad_model PRIVATE cxx_std_17)
  add_test(NAME dialpad_model COMMAND test_dialpad_model)
  add_executable(test_sip_address tests/test_sip_address.cpp)
  target_link_libraries(test_sip_address PRIVATE axv_sip_address)
  add_test(NAME sip_address COMMAND test_sip_address)
  add_executable(test_ui_help tests/test_ui_help.cpp)
  target_compile_features(test_ui_help PRIVATE cxx_std_17)
  add_test(NAME ui_help COMMAND test_ui_help)
  add_executable(test_account_form_feedback tests/test_account_form_feedback.cpp)
  target_compile_features(test_account_form_feedback PRIVATE cxx_std_17)
  add_test(NAME account_form_feedback COMMAND test_account_form_feedback)
  add_executable(test_destination_feedback tests/test_destination_feedback.cpp)
  target_compile_features(test_destination_feedback PRIVATE cxx_std_17)
  target_link_libraries(test_destination_feedback PRIVATE axv_sip_address)
  add_test(NAME destination_feedback COMMAND test_destination_feedback)
  add_executable(test_sip_runtime_observation tests/test_sip_runtime_observation.cpp)
  target_compile_features(test_sip_runtime_observation PRIVATE cxx_std_17)
  add_test(NAME sip_runtime_observation COMMAND test_sip_runtime_observation)
  add_executable(test_baresip_event_mailbox tests/test_baresip_event_mailbox.cpp)
  target_compile_features(test_baresip_event_mailbox PRIVATE cxx_std_17)
  add_test(NAME baresip_event_mailbox COMMAND test_baresip_event_mailbox)
  add_executable(test_sip_event_projection tests/test_sip_event_projection.cpp)
  target_compile_features(test_sip_event_projection PRIVATE cxx_std_17)
  add_test(NAME sip_event_projection COMMAND test_sip_event_projection)
  add_executable(test_sip_runtime_session_guard tests/test_sip_runtime_session_guard.cpp)
  target_compile_features(test_sip_runtime_session_guard PRIVATE cxx_std_17)
  add_test(NAME sip_runtime_session_guard COMMAND test_sip_runtime_session_guard)
  add_executable(test_dial_authorization tests/test_dial_authorization.cpp)
  target_compile_features(test_dial_authorization PRIVATE cxx_std_17)
  target_link_libraries(test_dial_authorization PRIVATE axv_sip_address)
  add_test(NAME dial_authorization COMMAND test_dial_authorization)
  add_executable(test_baresip_event_reducer tests/test_baresip_event_reducer.cpp)
  target_compile_features(test_baresip_event_reducer PRIVATE cxx_std_17)
  add_test(NAME baresip_event_reducer COMMAND test_baresip_event_reducer)
  add_executable(test_baresip_call_correlation tests/test_baresip_call_correlation.cpp)
  target_compile_features(test_baresip_call_correlation PRIVATE cxx_std_17)
  add_test(NAME baresip_call_correlation COMMAND test_baresip_call_correlation)
  add_executable(test_baresip_callback_ingress tests/test_baresip_callback_ingress.cpp)
  target_compile_features(test_baresip_callback_ingress PRIVATE cxx_std_17)
  add_test(NAME baresip_callback_ingress COMMAND test_baresip_callback_ingress)
  add_executable(test_baresip_callback_lifecycle tests/test_baresip_callback_lifecycle.cpp)
  target_compile_features(test_baresip_callback_lifecycle PRIVATE cxx_std_17)
  add_test(NAME baresip_callback_lifecycle COMMAND test_baresip_callback_lifecycle)
  add_executable(test_baresip_sdk_event_adapter
    tests/test_baresip_sdk_event_adapter.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_event_adapter PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_event_adapter PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_event_adapter COMMAND test_baresip_sdk_event_adapter)
  add_executable(test_baresip_sdk_registration_gateway
    tests/test_baresip_sdk_registration_gateway.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_registration_gateway PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_registration_gateway PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_registration_gateway COMMAND test_baresip_sdk_registration_gateway)
  add_executable(test_baresip_registration_watchdog tests/test_baresip_registration_watchdog.cpp)
  target_compile_features(test_baresip_registration_watchdog PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_watchdog COMMAND test_baresip_registration_watchdog)
  add_executable(test_baresip_native_thread_guard tests/test_baresip_native_thread_guard.cpp)
  target_compile_features(test_baresip_native_thread_guard PRIVATE cxx_std_17)
  add_test(NAME baresip_native_thread_guard COMMAND test_baresip_native_thread_guard)
  add_executable(test_baresip_registration_command_queue
    tests/test_baresip_registration_command_queue.cpp)
  target_compile_features(test_baresip_registration_command_queue PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_command_queue COMMAND test_baresip_registration_command_queue)
  add_executable(test_baresip_registration_command_dispatcher
    tests/test_baresip_registration_command_dispatcher.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_registration_command_dispatcher PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_registration_command_dispatcher PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_command_dispatcher COMMAND test_baresip_registration_command_dispatcher)
  add_executable(test_baresip_registration_command_races
    tests/test_baresip_registration_command_races.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_registration_command_races PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_registration_command_races PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_command_races COMMAND test_baresip_registration_command_races)
  add_executable(test_baresip_registration_lab_worker
    tests/test_baresip_registration_lab_worker.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_registration_lab_worker PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_registration_lab_worker PRIVATE cxx_std_17)
  add_test(NAME baresip_registration_lab_worker COMMAND test_baresip_registration_lab_worker)
  # ABI type-check is compiled with fake headers only, with no native SDK.
  add_executable(test_baresip_sdk_abi_contract tests/test_baresip_sdk_abi_contract.cpp)
  target_include_directories(test_baresip_sdk_abi_contract PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_abi_contract PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_abi_contract COMMAND test_baresip_sdk_abi_contract)
  add_test(NAME baresip_sdk_integrity_preflight
    COMMAND "${CMAKE_COMMAND}"
      "-DAXV_REPOSITORY_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DAXV_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}/sdk-integrity-cts"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/test_baresip_sdk_integrity.cmake")
  add_test(NAME libre_sdk_integrity_preflight
    COMMAND "${CMAKE_COMMAND}"
      "-DAXV_REPOSITORY_ROOT=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DAXV_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}/libre-integrity-cts"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/test_libre_sdk_integrity.cmake")
  set_tests_properties(baresip_sdk_integrity_preflight libre_sdk_integrity_preflight
    PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_registration_lab_worker PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_registration_command_races PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_registration_command_dispatcher PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_sdk_registration_gateway PROPERTIES TIMEOUT 15)
  set_tests_properties(baresip_sdk_event_adapter PROPERTIES TIMEOUT 15)
  add_executable(test_baresip_sdk_callback_epochs
    tests/test_baresip_sdk_callback_epochs.cpp
    src/baresip_sdk_event_adapter.cpp)
  target_include_directories(test_baresip_sdk_callback_epochs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fakes")
  target_compile_features(test_baresip_sdk_callback_epochs PRIVATE cxx_std_17)
  add_test(NAME baresip_sdk_callback_epochs COMMAND test_baresip_sdk_callback_epochs)
  add_executable(test_sip_engine tests/test_sip_engine.cpp)
  target_compile_features(test_sip_engine PRIVATE cxx_std_17)
  add_test(NAME sip_engine COMMAND test_sip_engine)
  add_executable(test_sip_account tests/test_sip_account.cpp)
  target_compile_features(test_sip_account PRIVATE cxx_std_17)
  add_test(NAME sip_account COMMAND test_sip_account)
  add_executable(test_sip_policy tests/test_sip_policy.cpp)
  target_compile_features(test_sip_policy PRIVATE cxx_std_17)
  add_test(NAME sip_policy COMMAND test_sip_policy)
  add_executable(test_sip_guardrails tests/test_sip_guardrails.cpp)
  target_compile_features(test_sip_guardrails PRIVATE cxx_std_17)
  add_test(NAME sip_guardrails COMMAND test_sip_guardrails)
  add_executable(test_sip_factory tests/test_sip_factory.cpp)
  target_compile_features(test_sip_factory PRIVATE cxx_std_17)
  add_test(NAME sip_factory COMMAND test_sip_factory)
  add_executable(test_baresip_probe tests/test_baresip_probe.cpp)
  target_compile_features(test_baresip_probe PRIVATE cxx_std_17)
  add_test(NAME baresip_probe COMMAND test_baresip_probe)
  add_executable(test_engine_lifecycle tests/test_engine_lifecycle.cpp)
  target_compile_features(test_engine_lifecycle PRIVATE cxx_std_17)
  add_test(NAME engine_lifecycle COMMAND test_engine_lifecycle)
  add_executable(test_audio_inventory tests/test_audio_inventory.cpp)
  target_compile_features(test_audio_inventory PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(test_audio_inventory PRIVATE /utf-8 /W4)
  endif()
  add_test(NAME audio_inventory COMMAND test_audio_inventory)
  add_executable(test_audio_selection_feedback tests/test_audio_selection_feedback.cpp)
  target_compile_features(test_audio_selection_feedback PRIVATE cxx_std_17)
  add_test(NAME audio_selection_feedback COMMAND test_audio_selection_feedback)
  add_executable(test_baresip_modules tests/test_baresip_modules.cpp)
  target_compile_features(test_baresip_modules PRIVATE cxx_std_17)
  add_test(NAME baresip_modules COMMAND test_baresip_modules)
  add_executable(test_baresip_profile tests/test_baresip_profile.cpp)
  target_compile_features(test_baresip_profile PRIVATE cxx_std_17)
  add_test(NAME baresip_profile COMMAND test_baresip_profile)
  add_executable(axv_test_child_process tests/child_process.cpp)
  target_compile_features(axv_test_child_process PRIVATE cxx_std_17)
  add_executable(axv_test_interactive_process tests/interactive_process.cpp)
  add_executable(axv_test_exit_process tests/exit_process.cpp)
  add_executable(test_shutdown_policy tests/test_shutdown_policy.cpp)
  target_compile_features(test_shutdown_policy PRIVATE cxx_std_17)
  add_test(NAME shutdown_policy COMMAND test_shutdown_policy)
  add_executable(test_windows_process tests/test_windows_process.cpp)
  target_compile_features(test_windows_process PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(test_windows_process PRIVATE /utf-8 /W4)
  endif()
  target_link_libraries(test_windows_process PRIVATE axv_windows_process)
  add_test(NAME windows_process COMMAND test_windows_process $<TARGET_FILE:axv_test_child_process> $<TARGET_FILE:axv_test_exit_process> $<TARGET_FILE:axv_test_interactive_process>)
endif()
set_target_properties(AXEVoice PROPERTIES OUTPUT_NAME "AXE-Voice")
