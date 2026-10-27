#pragma once
#include "windows.h"
inline constexpr DWORD COINIT_APARTMENTTHREADED=2, CLSCTX_INPROC_SERVER=1;
HRESULT CoInitializeEx(void*,DWORD);
void CoUninitialize();
HRESULT CoCreateInstance(REFIID,IUnknown*,DWORD,REFIID,void**);
