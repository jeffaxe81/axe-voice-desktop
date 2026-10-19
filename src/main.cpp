#include <windows.h>
#include "sip_address.h"
#include <string>

namespace {
constexpr int kCallButton = 1001;
constexpr int kNumberEdit = 1002;
HWND g_number = nullptr;
HWND g_status = nullptr;

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"AXE Voice  |  Desktop SIP",
            WS_CHILD | WS_VISIBLE, 20, 18, 340, 25, window, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"Número ou endereço SIP",
            WS_CHILD | WS_VISIBLE, 20, 60, 340, 20, window, nullptr, nullptr, nullptr);
        g_number = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 20, 86, 340, 30,
            window, reinterpret_cast<HMENU>(kNumberEdit), nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Chamar",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 20, 130, 110, 35,
            window, reinterpret_cast<HMENU>(kCallButton), nullptr, nullptr);
        g_status = CreateWindowW(L"STATIC", L"Motor SIP não conectado",
            WS_CHILD | WS_VISIBLE, 20, 188, 340, 28, window, nullptr, nullptr, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == kCallButton && HIWORD(wparam) == BN_CLICKED) {
            wchar_t number[256] = {};
            GetWindowTextW(g_number, number, 256);
            if (number[0] == L'\0') {
                SetWindowTextW(g_status, L"Informe um número ou SIP URI.");
            } else if (!axv::validate_address(number)) {
                SetWindowTextW(g_status, L"Destino SIP inválido.");
            } else {
                SetWindowTextW(g_status, L"Destino válido; integrar Baresip para chamar.");
            }
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    const wchar_t* className = L"AXEVoiceDesktopWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc)) return 1;
    HWND window = CreateWindowExW(0, className, L"AXE Voice — Áudio SIP",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 410, 300,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 2;
    ShowWindow(window, show);
    UpdateWindow(window);
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
