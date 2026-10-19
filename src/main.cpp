#include <windows.h>
#include "sip_address.h"
#include "sip_engine.h"
#include "sip_account.h"
#include "sip_status.h"
#include "call_policy.h"
#include "sip_diagnostics.h"
#include "sip_registration_policy.h"
#include "sip_security_policy.h"
#include "sip_engine_factory.h"
#include <string>

namespace {
constexpr int kCallButton = 1001;
constexpr int kNumberEdit = 1002;
constexpr int kValidateAccount = 1003;
constexpr int kServerEdit = 1004;
constexpr int kUserEdit = 1005;
constexpr int kPortEdit = 1006;
constexpr int kTlsCheckbox = 1007;
HWND g_number = nullptr;
HWND g_status = nullptr;
HWND g_server = nullptr;
HWND g_user = nullptr;
HWND g_port = nullptr;
HWND g_tls = nullptr;
axv::EngineSelection g_sip_selection = axv::create_sip_engine();

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
        CreateWindowW(L"STATIC", L"Servidor SIP", WS_CHILD | WS_VISIBLE,
            20, 235, 340, 20, window, nullptr, nullptr, nullptr);
        g_server = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            20, 257, 340, 27, window, reinterpret_cast<HMENU>(kServerEdit), nullptr, nullptr);
        CreateWindowW(L"STATIC", L"Usuário SIP", WS_CHILD | WS_VISIBLE,
            20, 292, 165, 20, window, nullptr, nullptr, nullptr);
        g_user = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            20, 313, 165, 27, window, reinterpret_cast<HMENU>(kUserEdit), nullptr, nullptr);
        CreateWindowW(L"STATIC", L"Porta", WS_CHILD | WS_VISIBLE,
            195, 292, 165, 20, window, nullptr, nullptr, nullptr);
        g_port = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"5060",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER,
            195, 313, 165, 27, window, reinterpret_cast<HMENU>(kPortEdit), nullptr, nullptr);
        g_tls = CreateWindowW(L"BUTTON", L"Usar TLS (quando suportado)",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            20, 348, 340, 28, window, reinterpret_cast<HMENU>(kTlsCheckbox), nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Validar conta (sem registrar)",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            20, 388, 340, 36, window, reinterpret_cast<HMENU>(kValidateAccount), nullptr, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == kValidateAccount && HIWORD(wparam) == BN_CLICKED) {
            wchar_t server[256] = {}, user[256] = {}, portText[16] = {};
            GetWindowTextW(g_server, server, 256);
            GetWindowTextW(g_user, user, 256);
            GetWindowTextW(g_port, portText, 16);
            unsigned long port = 0;
            bool digitsOnly = portText[0] != L'\0';
            for (const wchar_t* p = portText; *p; ++p) {
                if (*p < L'0' || *p > L'9') { digitsOnly = false; break; }
                port = port * 10 + static_cast<unsigned long>(*p - L'0');
                if (port > 65535) { digitsOnly = false; break; }
            }
            if (!digitsOnly || port == 0) {
                SetWindowTextW(g_status, L"Porta SIP inválida.");
                return 0;
            }
            axv::SipAccount account{server, user, static_cast<unsigned short>(port),
                SendMessageW(g_tls, BM_GETCHECK, 0, 0) == BST_CHECKED};
            if (axv::validate_account(account) != axv::AccountError::None) {
                SetWindowTextW(g_status, L"Servidor ou usuário SIP inválido.");
            } else {
                const auto decision = axv::evaluate_registration(account, g_sip_selection.engine->status());
                if (decision == axv::RegistrationDecision::EngineUnavailable) {
                    SetWindowTextW(g_status, L"Conta válida. Motor Baresip não conectado.");
                } else {
                    SetWindowTextW(g_status, L"Dados válidos. Registro SIP ainda não executado.");
                }
            }
            return 0;
        }
        if (LOWORD(wparam) == kCallButton && HIWORD(wparam) == BN_CLICKED) {
            wchar_t number[256] = {};
            GetWindowTextW(g_number, number, 256);
            if (number[0] == L'\0') {
                SetWindowTextW(g_status, L"Informe um número ou SIP URI.");
            } else if (!axv::validate_address(number)) {
                SetWindowTextW(g_status, L"Destino SIP inválido.");
            } else {
                if (!axv::can_apply_call_action(g_sip_selection.engine->status(), axv::CallAction::Dial)) {
                    SetWindowTextW(g_status, axv::diagnostic_text(axv::DiagnosticCode::RegistrationPending));
                } else if (!g_sip_selection.engine->start_call(axv::validate_address(number).normalized)) {
                    SetWindowTextW(g_status, g_sip_selection.engine->diagnostic().c_str());
                }
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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 410, 510,
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
