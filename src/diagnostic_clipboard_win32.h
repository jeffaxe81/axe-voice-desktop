#pragma once
#include <windows.h>
#include "call_readiness.h"
namespace axv {
// User-triggered local copy only. Success transfers ownership to Windows.
bool copy_call_diagnostic(HWND owner,const CallReadiness& state);
}
