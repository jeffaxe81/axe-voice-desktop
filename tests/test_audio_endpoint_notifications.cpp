#include "../src/audio_endpoint_notifications_win32.h"
#include <windows.h>

namespace {
constexpr UINT kChanged = WM_APP + 77;
int seen = 0;
LRESULT CALLBACK on_test_event(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == kChanged) { ++seen; return 0; }
    return DefWindowProcW(hwnd, msg, w, l);
}
void pump() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
}
int main() {
    WNDCLASSW klass{};
    klass.lpfnWndProc = on_test_event;
    klass.hInstance = GetModuleHandleW(nullptr);
    klass.lpszClassName = L"AXEVoiceSyntheticAudioNotification";
    if (!RegisterClassW(&klass)) return 1;
    HWND hwnd = CreateWindowExW(0, klass.lpszClassName, L"", WS_OVERLAPPED,
        0, 0, 100, 100, nullptr, nullptr, klass.hInstance, nullptr);
    if (!hwnd) return 2;
    auto* sink = new axv::AudioEndpointNotificationSink(hwnd, kChanged);
    void* unknown = nullptr;
    if (FAILED(sink->QueryInterface(__uuidof(IUnknown), &unknown)) || !unknown) return 3;
    static_cast<IUnknown*>(unknown)->Release();
    if (sink->QueryInterface(__uuidof(IMMDeviceEnumerator), &unknown) != E_NOINTERFACE)
        return 4;

    PROPERTYKEY property{};
    sink->OnDeviceAdded(L"secret-id-1");
    sink->OnDeviceRemoved(L"secret-id-2");
    sink->OnDeviceStateChanged(L"secret-id-3", DEVICE_STATE_DISABLED);
    sink->OnDefaultDeviceChanged(eRender, eCommunications, L"secret-id-4");
    sink->OnPropertyValueChanged(L"secret-id-5", property);
    pump();
    if (seen != 5) return 5;
    sink->detach();
    sink->OnDefaultDeviceChanged(eCapture, eCommunications, L"secret-id-6");
    pump();
    if (seen != 5) return 6;
    if (sink->Release() != 0) return 7;
    DestroyWindow(hwnd);
    UnregisterClassW(klass.lpszClassName, klass.hInstance);
    return 0;
}
