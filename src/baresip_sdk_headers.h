#pragma once
// Keep C SDK logging names out of the C++/MSVC standard-library namespace.
#pragma push_macro("debug")
#pragma push_macro("info")
#pragma push_macro("warning")
#pragma push_macro("loglv")
#pragma push_macro("log")
#undef debug
#undef info
#undef warning
#undef loglv
#undef log
#include <cmath>
#include <re.h>
// Baresip 4.12 uses `log` only as a C struct tag and handler argument type.
// Renaming that tag here preserves its layout and extern-C symbol names,
// while avoiding MSVC's global <cmath> log function-template collision.
#define log axv_baresip_log_record
#include <baresip.h>
#undef log
#undef debug
#undef info
#undef warning
#undef loglv
#pragma pop_macro("log")
#pragma pop_macro("loglv")
#pragma pop_macro("warning")
#pragma pop_macro("info")
#pragma pop_macro("debug")
