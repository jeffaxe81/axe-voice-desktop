#pragma once
#include "windows.h"
// Deliberately minimal source-level doubles; these do not model the Windows ABI.
struct PROPERTYKEY { GUID fmtid; DWORD pid; };
enum EDataFlow { eRender, eCapture, eAll };
enum ERole { eConsole, eMultimedia, eCommunications };
inline constexpr GUID IID_IMMNotificationClient{2}, IID_IMMDeviceEnumerator{3}, CLSID_MMDeviceEnumerator{4};
struct IMMNotificationClient : IUnknown {
 virtual HRESULT OnDeviceStateChanged(LPCWSTR,DWORD)=0;
 virtual HRESULT OnDeviceAdded(LPCWSTR)=0;
 virtual HRESULT OnDeviceRemoved(LPCWSTR)=0;
 virtual HRESULT OnDefaultDeviceChanged(EDataFlow,ERole,LPCWSTR)=0;
 virtual HRESULT OnPropertyValueChanged(LPCWSTR,const PROPERTYKEY)=0;
};
struct IMMDeviceEnumerator : IUnknown {
 virtual HRESULT RegisterEndpointNotificationCallback(IMMNotificationClient*)=0;
 virtual HRESULT UnregisterEndpointNotificationCallback(IMMNotificationClient*)=0;
};

// Match the production header UUID lookup without requiring Windows ABI.
#define __uuidof(type) AXV_FAKE_UUID_##type
inline constexpr GUID AXV_FAKE_UUID_IMMNotificationClient = IID_IMMNotificationClient;
inline constexpr GUID AXV_FAKE_UUID_IMMDeviceEnumerator = IID_IMMDeviceEnumerator;
inline constexpr GUID AXV_FAKE_UUID_MMDeviceEnumerator = CLSID_MMDeviceEnumerator;
