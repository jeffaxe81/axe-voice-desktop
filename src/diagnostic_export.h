#pragma once
#include "call_readiness.h"
#include "native_security_status.h"
namespace axv {
// Boolean-only input prevents account/destination/device values entering export.
inline std::wstring format_diagnostic_export(const CallReadiness& state,const NativeSecurityStatus& security={}){
 const auto source=std::wstring(L"AXE Voice candidate 0.63.0\n")+format_call_readiness(state)+L"\n\n"+format_native_security_status(security);
 std::wstring result;result.reserve(source.size()+32);
 for(const auto ch:source){if(ch==L'\n')result+=L'\r';result+=ch;}
 return result;
}
}
