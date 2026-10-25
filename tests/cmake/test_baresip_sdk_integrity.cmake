cmake_minimum_required(VERSION 3.20)
# Synthetic byte fixtures for the SHA/version gate only. They are not SDKs.
if(NOT DEFINED AXV_REPOSITORY_ROOT OR NOT DEFINED AXV_SCRATCH OR
   "${AXV_REPOSITORY_ROOT}" STREQUAL "" OR "${AXV_SCRATCH}" STREQUAL "")
  message(FATAL_ERROR "Missing repository root or isolated test scratch directory")
endif()
set(_sdk_dir "${AXV_SCRATCH}/synthetic-sdk")
file(MAKE_DIRECTORY "${_sdk_dir}")
set(_inc "${_sdk_dir}")
set(_header "${_inc}/baresip.h")
set(_lib "${_sdk_dir}/baresip.lib")
file(WRITE "${_header}" "#define BARESIP_VERSION \"synthetic-test-1\"\n")
file(WRITE "${_lib}" "Synthetic non-functional library bytes; not an SDK\n")
file(SHA256 "${_header}" _header_sha)
file(SHA256 "${_lib}" _library_sha)

function(check_case _name _outcome _include _library _version _sha_header _sha_library _error_fragment)
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      "-DAXV_REPOSITORY_ROOT=${AXV_REPOSITORY_ROOT}"
      "-DAXV_BARESIP_INCLUDE_DIR=${_include}"
      "-DAXV_BARESIP_LIBRARY=${_library}"
      "-DAXV_BARESIP_VERSION=${_version}"
      "-DAXV_BARESIP_HEADER_SHA256=${_sha_header}"
      "-DAXV_BARESIP_LIBRARY_SHA256=${_sha_library}"
      -P "${AXV_REPOSITORY_ROOT}/cmake/axv_verify_baresip_sdk.cmake"
    RESULT_VARIABLE _exit OUTPUT_VARIABLE _out ERROR_VARIABLE _err
  )
  if(_outcome STREQUAL "pass")
    if(NOT _exit EQUAL 0)
      message(FATAL_ERROR "${_name}: expected success; got exit ${_exit}\n${_err}")
    endif()
  elseif(_exit EQUAL 0)
    message(FATAL_ERROR "${_name}: expected preflight refusal, got success")
  endif()
  if(NOT "${_error_fragment}" STREQUAL "")
    string(FIND "${_err}" "${_error_fragment}" _position)
    if(_position EQUAL -1)
      message(FATAL_ERROR "${_name}: expected diagnostic '${_error_fragment}'\n${_err}")
    endif()
  endif()
endfunction()

string(TOUPPER "${_header_sha}" _upper_header_sha)
check_case(valid pass "${_inc}" "${_lib}" synthetic-test-1
           "${_upper_header_sha}" "${_library_sha}" "")
check_case(header_changed fail "${_inc}" "${_lib}" synthetic-test-1
           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
           "${_library_sha}" "SDK header SHA-256 mismatch")
check_case(library_changed fail "${_inc}" "${_lib}" synthetic-test-1
           "${_header_sha}"
           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
           "SDK library SHA-256 mismatch")
check_case(missing_hash fail "${_inc}" "${_lib}" synthetic-test-1
           "" "${_library_sha}" "missing AXV_BARESIP_HEADER_SHA256")
check_case(bad_hex fail "${_inc}" "${_lib}" synthetic-test-1
           "not-a-sha" "${_library_sha}" "SHA-256 must be exactly 64")
check_case(wrong_version fail "${_inc}" "${_lib}" synthetic-test-2
           "${_header_sha}" "${_library_sha}" "SDK header version mismatch")
check_case(missing_library fail "${_inc}" "${_sdk_dir}/missing.lib" synthetic-test-1
           "${_header_sha}" "${_library_sha}" "SDK library not found")
check_case(fake_sdk_header fail "${AXV_REPOSITORY_ROOT}/tests/fakes" "${_lib}" synthetic-test-1
           "${_header_sha}" "${_library_sha}" "test SDK header is forbidden")
file(WRITE "${_lib}" "")
check_case(empty_library fail "${_inc}" "${_lib}" synthetic-test-1
           "${_header_sha}" "${_library_sha}" "SDK library must be non-empty")
file(REMOVE_RECURSE "${AXV_SCRATCH}")
message(STATUS "AXE SDK preflight: 9 synthetic positive/negative cases passed")
