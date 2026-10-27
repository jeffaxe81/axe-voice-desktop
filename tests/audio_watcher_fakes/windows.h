#pragma once
#include <cstdint>
using HRESULT = std::int32_t;
using ULONG = std::uint32_t;
using DWORD = std::uint32_t;
using LPCWSTR = const wchar_t*;
#define STDMETHODCALLTYPE
inline constexpr HRESULT S_OK=0, S_FALSE=1;
inline constexpr HRESULT E_FAIL=static_cast<HRESULT>(0x80004005u);
inline constexpr HRESULT E_POINTER=static_cast<HRESULT>(0x80004003u);
inline constexpr HRESULT E_NOINTERFACE=static_cast<HRESULT>(0x80004002u);
inline constexpr HRESULT RPC_E_CHANGED_MODE=static_cast<HRESULT>(0x80010106u);
#define FAILED(hr) ((hr)<0)
struct GUID { unsigned value; };
using REFIID = const GUID&;
inline bool IsEqualIID(REFIID a, REFIID b) { return a.value==b.value; }
inline constexpr GUID IID_IUnknown{1};
struct IUnknown {
 virtual HRESULT QueryInterface(REFIID,void**)=0;
 virtual ULONG AddRef()=0;
 virtual ULONG Release()=0;
};
