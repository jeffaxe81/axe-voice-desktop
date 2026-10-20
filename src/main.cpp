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
#include "baresip_probe.h"
#include "audio_devices.h"
#include "windows_process.h"
#include <string>

namespace {
constexpr int kCallButton = 1001;
constexpr int kNumberEdit = 1002;
constexpr int kValidateAccount = 1003;
constexpr int kServerEdit = 1004;
constexpr int kUserEdit = 1005;
constexpr int kPortEdit = 1006;
constexpr int kTlsCheckbox = 1007;
constexpr int kProbeButton = 1008;
constexpr int kAudioDevicesButton = 1009;
constexpr int kStartBaresipButton = 1010;
constexpr int kStopBaresipButton = 1011;
constexpr UINT_PTR kProcessTimer = 1012;
HWND g_number = nullptr;
HWND g_status = nullptr;
HWND g_server = nullptr;
HWND g_user = nullptr;
HWND g_port = nullptr;
HWND g_tls = nullptr;
HWND g_audio_status = nullptr;
HWND g_process_status = nullptr;
axv::WindowsProcess g_baresip_process;
axv::ProcessStatus g_last_process_state = axv::ProcessStatus::Stopped;
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
        CreateWindowW(L"BUTTON", L"Verificar Baresip local",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            20, 432, 340, 32, window, reinterpret_cast<HMENU>(kProbeButton), nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Verificar microfone e saída de áudio",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            20, 474, 340, 32, window, reinterpret_cast<HMENU>(kAudioDevicesButton), nullptr, nullptr);
        g_audio_status = CreateWindowW(L"STATIC", L"Áudio ainda não inspecionado.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 514, 350, 55, window, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Iniciar Baresip",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            20, 575, 163, 34, window, reinterpret_cast<HMENU>(kStartBaresipButton), nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Parar motor",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            197, 575, 163, 34, window, reinterpret_cast<HMENU>(kStopBaresipButton), nullptr, nullptr);
        g_process_status = CreateWindowW(L"STATIC", L"Processo Baresip parado. Registro SIP indisponível.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 617, 350, 56, window, nullptr, nullptr, nullptr);
        SetTimer(window, kProcessTimer, 1000, nullptr);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == kStartBaresipButton && HIWORD(wparam) == BN_CLICKED) {
            wchar_t application[MAX_PATH] = {};
            const DWORD length = GetModuleFileNameW(nullptr, application, MAX_PATH);
            if (length == 0 || length >= MAX_PATH) {
                SetWindowTextW(g_process_status, L"Falha ao localizar o executável do AXE Voice.");
                return 0;
            }
            const std::filesystem::path candidate =
                std::filesystem::path(application).parent_path() / L"baresip.exe";
            const auto probe = axv::probe_baresip_executable(candidate);
            if (probe.status != axv::BaresipProbeStatus::PresentUnverified) {
                SetWindowTextW(g_process_status, probe.message.c_str());
                return 0;
            }
            if (g_baresip_process.poll() == axv::ProcessStatus::Running) {
                SetWindowTextW(g_process_status, L"O processo Baresip já está em execução.");
                return 0;
            }
            if (MessageBoxW(window,
                L"Iniciar baresip.exe desta pasta? Execute somente arquivos de origem e hash verificados. "
                L"A aplicação não confirma identidade, registro SIP ou chamadas.",
                L"Confirmar inicialização do Baresip",
                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return 0;
            if (g_baresip_process.start(candidate)) {
                g_last_process_state = axv::ProcessStatus::Running;
                SetWindowTextW(g_process_status, L"Processo Baresip iniciado. Registro SIP NÃO confirmado.");
            } else {
                g_last_process_state = axv::ProcessStatus::Failed;
                const std::wstring error = L"Falha ao iniciar Baresip. Erro Windows: " +
                    std::to_wstring(g_baresip_process.last_error());
                SetWindowTextW(g_process_status, error.c_str());
            }
            return 0;
        }
        if (LOWORD(wparam) == kStopBaresipButton && HIWORD(wparam) == BN_CLICKED) {
            const bool stopped = g_baresip_process.stop();
            g_last_process_state = axv::ProcessStatus::Stopped;
            SetWindowTextW(g_process_status,
                stopped ? L"Processo encerrado. Encerramento forçado: não usar em produção."
                        : L"Falha ao confirmar encerramento do processo.");
            return 0;
        }
        if (LOWORD(wparam) == kAudioDevicesButton && HIWORD(wparam) == BN_CLICKED) {
            const axv::AudioInventory inventory = axv::inspect_windows_audio_devices();
            const std::wstring summary = axv::audio_inventory_summary(inventory);
            SetWindowTextW(g_audio_status, summary.c_str());
            return 0;
        }
        if (LOWORD(wparam) == kProbeButton && HIWORD(wparam) == BN_CLICKED) {
            wchar_t executable[MAX_PATH] = {};
            DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
            if (length == 0 || length >= MAX_PATH) {
                SetWindowTextW(g_status, L"Não foi possível localizar a pasta do AXE Voice.");
                return 0;
            }
            std::filesystem::path candidate(executable);
            candidate = candidate.parent_path() / L"baresip.exe";
            const auto probe = axv::probe_baresip_executable(candidate);
            SetWindowTextW(g_status, probe.message.c_str());
            return 0;
        }
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
    case WM_TIMER:
        if (wparam == kProcessTimer) {
            const auto state = g_baresip_process.poll();
            if (state != g_last_process_state) {
                g_last_process_state = state;
                if (state == axv::ProcessStatus::Exited) {
                    SetWindowTextW(g_process_status,
                        L"Baresip encerrou. Consulte a configuração e dependências do motor.");
                } else if (state == axv::ProcessStatus::Failed) {
                    SetWindowTextW(g_process_status, L"Falha ao consultar processo Baresip.");
                }
            }
            return 0;
        }
        break;
    case WM_DESTROY:
        KillTimer(window, kProcessTimer);
        g_baresip_process.stop();
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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 410, 760,
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
