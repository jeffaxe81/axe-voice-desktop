# AXE Voice — offline integrity gate for an explicitly supplied Baresip SDK.
# Included ONLY when AXV_BUILD_NATIVE_BARESIP_ADAPTER=ON.
# Matching hashes pin local bytes; they do NOT authenticate the distributor.
foreach(_required IN ITEMS AXV_BARESIP_INCLUDE_DIR AXV_BARESIP_LIBRARY
                           AXV_BARESIP_HEADER_SHA256 AXV_BARESIP_LIBRARY_SHA256
                           AXV_BARESIP_VERSION AXV_REPOSITORY_ROOT)
  if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "AXE SDK preflight: missing ${_required}")
  endif()
endforeach()

set(_sdk_header "${AXV_BARESIP_INCLUDE_DIR}/baresip.h")
if(NOT EXISTS "${_sdk_header}" OR IS_DIRECTORY "${_sdk_header}")
  message(FATAL_ERROR "AXE SDK preflight: SDK header not found")
endif()
if(NOT EXISTS "${AXV_BARESIP_LIBRARY}" OR IS_DIRECTORY "${AXV_BARESIP_LIBRARY}")
  message(FATAL_ERROR "AXE SDK preflight: SDK library not found")
endif()

file(REAL_PATH "${_sdk_header}" _sdk_header_real)
file(REAL_PATH "${AXV_BARESIP_LIBRARY}" _sdk_library_real)
file(REAL_PATH "${AXV_REPOSITORY_ROOT}/tests/fakes/baresip.h" _fixture_header)
if(WIN32)
  string(TOLOWER "${_sdk_header_real}" _sdk_header_real)
  string(TOLOWER "${_fixture_header}" _fixture_header)
endif()
if("${_sdk_header_real}" STREQUAL "${_fixture_header}")
  message(FATAL_ERROR "AXE SDK preflight: test SDK header is forbidden")
endif()

get_filename_component(_lib_ext "${_sdk_library_real}" EXT)
string(TOLOWER "${_lib_ext}" _lib_ext)
if(NOT _lib_ext STREQUAL ".lib")
  message(FATAL_ERROR "AXE SDK preflight: expected an MSVC .lib library")
endif()
file(SIZE "${_sdk_library_real}" _lib_size)
if(_lib_size EQUAL 0)
  message(FATAL_ERROR "AXE SDK preflight: SDK library must be non-empty")
endif()

foreach(_kind IN ITEMS HEADER LIBRARY)
  set(_pin "${AXV_BARESIP_${_kind}_SHA256}")
  string(LENGTH "${_pin}" _pin_size)
  if(NOT _pin_size EQUAL 64 OR NOT _pin MATCHES "^[0-9a-fA-F]+$")
    message(FATAL_ERROR "AXE SDK preflight: SDK ${_kind} SHA-256 must be exactly 64 hex characters")
  endif()
endforeach()

file(SHA256 "${_sdk_header_real}" _actual_header_sha)
file(SHA256 "${_sdk_library_real}" _actual_library_sha)
string(TOLOWER "${AXV_BARESIP_HEADER_SHA256}" _header_pin)
string(TOLOWER "${AXV_BARESIP_LIBRARY_SHA256}" _library_pin)
if(NOT _actual_header_sha STREQUAL _header_pin)
  message(FATAL_ERROR "AXE SDK preflight: SDK header SHA-256 mismatch")
endif()
if(NOT _actual_library_sha STREQUAL _library_pin)
  message(FATAL_ERROR "AXE SDK preflight: SDK library SHA-256 mismatch")
endif()

file(STRINGS "${_sdk_header_real}" _version_lines
  REGEX "^[ \t]*#[ \t]*define[ \t]+BARESIP_VERSION[ \t]+\"[^\"]+\"")
list(LENGTH _version_lines _version_count)
if(NOT _version_count EQUAL 1)
  message(FATAL_ERROR "AXE SDK preflight: exactly one BARESIP_VERSION define is required")
endif()
list(GET _version_lines 0 _version_line)
string(REGEX REPLACE ".*BARESIP_VERSION[ \t]+\"([^\"]+)\".*" "\\1"
  _actual_version "${_version_line}")
if(NOT _actual_version STREQUAL AXV_BARESIP_VERSION)
  message(FATAL_ERROR "AXE SDK preflight: SDK header version mismatch")
endif()

message(STATUS "AXE SDK preflight: pinned version ${_actual_version}, header and library SHA-256 matched (not provenance attestation)")
