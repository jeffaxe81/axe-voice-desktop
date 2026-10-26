#pragma once
// Isolated WinAPI boundary for ownership tests; never linked to system clipboard.
#include <cstddef>
using HWND=void*;
using HGLOBAL=void*;
using HANDLE=void*;
constexpr unsigned GMEM_MOVEABLE=2,CF_UNICODETEXT=13;
extern "C" {
HGLOBAL GlobalAlloc(unsigned,std::size_t);
void* GlobalLock(HGLOBAL);
int GlobalUnlock(HGLOBAL);
HGLOBAL GlobalFree(HGLOBAL);
int OpenClipboard(HWND);
int EmptyClipboard();
HANDLE SetClipboardData(unsigned,HANDLE);
int CloseClipboard();
}
