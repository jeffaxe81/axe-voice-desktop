#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "diagnostic_clipboard_win32.h"
#include "diagnostic_export.h"
#include <cstring>
namespace axv {
bool copy_call_diagnostic(HWND owner,const CallReadiness& state,const NativeSecurityStatus& security){
 if(!owner)return false;
 const auto text=format_diagnostic_export(state,security);
 const auto bytes=(text.size()+1)*sizeof(wchar_t);
 const auto memory=GlobalAlloc(GMEM_MOVEABLE,bytes);
 if(!memory)return false;
 auto* data=GlobalLock(memory);
 if(!data){GlobalFree(memory);return false;}
 std::memcpy(data,text.c_str(),bytes);
 GlobalUnlock(memory);
 if(!OpenClipboard(owner)){GlobalFree(memory);return false;}
 const bool copied=EmptyClipboard() && SetClipboardData(CF_UNICODETEXT,memory)!=nullptr;
 CloseClipboard();
 if(!copied)GlobalFree(memory);
 return copied;
}
}
