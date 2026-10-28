#pragma once
#include <cstddef>
#include <cstdint>
struct RegistrationFakeControl;
using HWND = RegistrationFakeControl*;
using UINT = unsigned;
using DWORD = unsigned long;
using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
using LRESULT = std::intptr_t;
constexpr unsigned CP_UTF8=65001, WC_ERR_INVALID_CHARS=0x80, BM_GETCHECK=0x00f0;
constexpr int BST_CHECKED=1;
int GetWindowTextW(HWND,wchar_t*,int);
int SetWindowTextW(HWND,const wchar_t*);
LRESULT SendMessageW(HWND,UINT,WPARAM,LPARAM);
int WideCharToMultiByte(UINT,DWORD,const wchar_t*,int,char*,int,const char*,int*);
void* SecureZeroMemory(void*,std::size_t);
