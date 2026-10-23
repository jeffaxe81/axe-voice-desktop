#include "../src/audio_endpoint_watcher.h"
#include <windows.h>
#include <cassert>

namespace {
constexpr UINT kAudioSignal = WM_APP + 55;
LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM, LPARAM) {
    if (message == kAudioSignal) {
        auto* count = reinterpret_cast<int*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (count) ++*count;
        return 0;
    }
    return DefWindowProcW(window, message, 0, 0);
}
void drain(HWND window) {
    MSG message{};
    while (PeekMessageW(&message, window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}
}
int main() {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = L"AXEVoiceAudioSinkTest";
    wc.lpfnWndProc = Proc;
    if (!RegisterClassW(&wc)) return 1;
    HWND window = CreateWindowExW(0, wc.lpszClassName, L"",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
    if (!window) return 2;
    int notices = 0;
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&notices));
    auto* sink = new axv::AudioEndpointNotificationSink(window, kAudioSignal);
    void* iface = nullptr;
    if (sink->QueryInterface(__uuidof(IMMNotificationClient), &iface) != S_OK ||
        iface == nullptr) return 3;
    static_cast<IMMNotificationClient*>(iface)->Release();
    iface = nullptr;
    if (sink->QueryInterface(__uuidof(IUnknown), &iface) != S_OK || !iface) return 4;
    static_cast<IUnknown*>(iface)->Release();
    if (sink->QueryInterface(__uuidof(IMMDeviceEnumerator), &iface) != E_NOINTERFACE ||
        iface) return 5;
    if (sink->QueryInterface(__uuidof(IUnknown), nullptr) != E_POINTER) return 6;
    sink->OnDefaultDeviceChanged(eCapture, eConsole, L"private-id");
    sink->OnDefaultDeviceChanged(eRender, eMultimedia, L"private-id");
    sink->OnDefaultDeviceChanged(eCapture, eCommunications, L"private-id");
    sink->OnDefaultDeviceChanged(eRender, eCommunications, nullptr);
    sink->OnDeviceAdded(L"private-id");
    sink->OnDeviceRemoved(L"private-id");
    sink->OnDeviceStateChanged(L"private-id", DEVICE_STATE_UNPLUGGED);
    PROPERTYKEY key{};
    sink->OnPropertyValueChanged(L"private-id", key);
    drain(window);
    if (notices != 6) return 7;
    if (sink->Release() != 0) return 8;
    DestroyWindow(window);
    UnregisterClassW(wc.lpszClassName, instance);
    return 0;
}
