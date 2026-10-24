cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED AXV_REPOSITORY_ROOT OR NOT DEFINED AXV_SCRATCH)
  message(FATAL_ERROR "Libre preflight tests require repository root and scratch")
endif()
set(_dir "${AXV_SCRATCH}/synthetic-libre")
file(MAKE_DIRECTORY "${_dir}")
file(WRITE "${_dir}/re.h" "/* synthetic ONLY */\n")
file(WRITE "${_dir}/libre.lib" "synthetic non-functional lib\n")
file(SHA256 "${_dir}/re.h" _h)
file(SHA256 "${_dir}/libre.lib" _l)

function(test_preflight label should_pass inc lib hp lp diagnostic)
  execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DAXV_REPOSITORY_ROOT=${AXV_REPOSITORY_ROOT}"
    "-DAXV_LIBRE_INCLUDE_DIR=${inc}"
    "-DAXV_LIBRE_LIBRARY=${lib}"
    "-DAXV_LIBRE_HEADER_SHA256=${hp}"
    "-DAXV_LIBRE_LIBRARY_SHA256=${lp}"
    -P "${AXV_REPOSITORY_ROOT}/cmake/axv_verify_libre_sdk.cmake"
    RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _error)
  if(should_pass AND NOT _result EQUAL 0)
    message(FATAL_ERROR "${label}: unexpectedly rejected: ${_error}")
  endif()
  if(NOT should_pass AND _result EQUAL 0)
    message(FATAL_ERROR "${label}: improperly accepted")
  endif()
  if(NOT "${diagnostic}" STREQUAL "")
    string(FIND "${_error}" "${diagnostic}" _at)
    if(_at LESS 0)
      message(FATAL_ERROR "${label}: expected ${diagnostic}; got ${_error}")
    endif()
  endif()
endfunction()
test_preflight(valid TRUE "${_dir}" "${_dir}/libre.lib" "${_h}" "${_l}" "")
test_preflight(wrong_header FALSE "${_dir}" "${_dir}/libre.lib"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" "${_l}" "re.h SHA-256 mismatch")
test_preflight(wrong_library FALSE "${_dir}" "${_dir}/libre.lib"
  "${_h}" "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" "libre library SHA-256 mismatch")
test_preflight(missing_pin FALSE "${_dir}" "${_dir}/libre.lib" "" "${_l}" "missing AXV_LIBRE_HEADER_SHA256")
test_preflight(fake_header FALSE "${AXV_REPOSITORY_ROOT}/tests/fakes" "${_dir}/libre.lib"
  "${_h}" "${_l}" "test libre header is forbidden")
file(WRITE "${_dir}/libre.lib" "")
test_preflight(empty_lib FALSE "${_dir}" "${_dir}/libre.lib" "${_h}" "${_l}" "library must be non-empty")
file(REMOVE_RECURSE "${AXV_SCRATCH}")
message(STATUS "AXE libre preflight: 6 synthetic cases passed")
