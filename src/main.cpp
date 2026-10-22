#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include "baresip_profile.h"
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
#include "baresip_modules.h"
#include "audio_devices.h"
#include "windows_process.h"
#include "shutdown_policy.h"
#include <string>

#include "dialpad_model.h"
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <cwchar>
#include <iterator>


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
constexpr int kShowDial = 2001;
constexpr int kShowSettings = 2002;
constexpr int kTheme = 2003;
constexpr int kPin = 2004;
constexpr int kDigitStart = 2100;
constexpr int kBackspace = 2201;
constexpr int kClear = 2202;
constexpr int kMuteFuture = 2301;
constexpr int kHoldFuture = 2302;
constexpr int kTransferFuture = 2303;
constexpr int kHangupFuture = 2304;
constexpr int kFocusNumber = 2305;
constexpr UINT kTrayCallback = WM_APP + 41;
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayOpen = 3001;
constexpr UINT kTrayExit = 3002;
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
axv::ShutdownDeadline g_shutdown;
bool g_close_requested = false;
constexpr std::uint64_t kGracePeriodMs = 1500;
axv::EngineSelection g_sip_selection = axv::create_sip_engine();


bool g_dark = true;
bool g_pinned = false;
bool g_tray_visible = false;
constexpr wchar_t kPrefsKey[] = L"Software\\AXEVoice\\Desktop";
int g_page = 0; // 0 = dial, 1 = settings
std::vector<HWND> g_dial_controls;
std::vector<HWND> g_settings_controls;
HFONT g_font_regular = nullptr;
HFONT g_font_title = nullptr;
HFONT g_font_small = nullptr;
HFONT g_font_digit = nullptr;
HBRUSH g_background_brush = nullptr;
HBRUSH g_field_brush = nullptr;


void load_ui_preferences() {
    DWORD value = 0, size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kPrefsKey, L"DarkTheme", RRF_RT_REG_DWORD,
                     nullptr, &value, &size) == ERROR_SUCCESS) {
        g_dark = value != 0;
    }
    value = 0; size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kPrefsKey, L"AlwaysOnTop", RRF_RT_REG_DWORD,
                     nullptr, &value, &size) == ERROR_SUCCESS) {
        g_pinned = value != 0;
    }
}
void save_ui_preferences() {
    HKEY key = nullptr;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kPrefsKey, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, &disposition) != ERROR_SUCCESS) return;
    const DWORD theme = g_dark ? 1u : 0u;
    const DWORD pinned = g_pinned ? 1u : 0u;
    RegSetValueExW(key, L"DarkTheme", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&theme), sizeof(theme));
    RegSetValueExW(key, L"AlwaysOnTop", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&pinned), sizeof(pinned));
    RegCloseKey(key);
}

NOTIFYICONDATAW create_tray_data(HWND window) {
    NOTIFYICONDATAW tray = {};
    tray.cbSize = sizeof(tray);
    tray.hWnd = window;
    tray.uID = kTrayIconId;
    tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tray.uCallbackMessage = kTrayCallback;
    tray.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    const wchar_t caption[] = L"AXE Voice - laboratorio";
    std::copy(std::begin(caption), std::end(caption), std::begin(tray.szTip));
    return tray;
}
bool install_tray(HWND window) {
    if (g_tray_visible) return true;
    auto data = create_tray_data(window);
    g_tray_visible = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
    if (g_tray_visible) {
        data.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &data);
    }
    return g_tray_visible;
}
void remove_tray(HWND window) {
    if (!g_tray_visible) return;
    auto data = create_tray_data(window);
    Shell_NotifyIconW(NIM_DELETE, &data);
    g_tray_visible = false;
}
void restore_tray(HWND window) {
    remove_tray(window);
    ShowWindow(window, SW_RESTORE);
    SetForegroundWindow(window);
}
BOOL CALLBACK repaint_child(HWND child, LPARAM) {
    InvalidateRect(child, nullptr, TRUE);
    return TRUE;
}
COLORREF background_color() { return g_dark ? RGB(14, 22, 38) : RGB(245, 248, 252); }
COLORREF panel_color() { return g_dark ? RGB(25, 38, 58) : RGB(255, 255, 255); }
COLORREF field_color() { return g_dark ? RGB(31, 48, 73) : RGB(238, 244, 249); }
COLORREF text_color() { return g_dark ? RGB(236, 244, 255) : RGB(20, 38, 62); }
COLORREF muted_color() { return g_dark ? RGB(157, 176, 200) : RGB(91, 108, 128); }
COLORREF accent_color() { return RGB(38, 178, 198); }
COLORREF line_color() { return g_dark ? RGB(57, 74, 97) : RGB(207, 218, 230); }

void refresh_brushes() {
    if (g_background_brush) DeleteObject(g_background_brush);
    if (g_field_brush) DeleteObject(g_field_brush);
    g_background_brush = CreateSolidBrush(background_color());
    g_field_brush = CreateSolidBrush(field_color());
}
HFONT create_font(int size, int weight) {
    return CreateFontW(-size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

HWND make_control(HWND root, LPCWSTR type, LPCWSTR title, DWORD style,
                  int x, int y, int width, int height, int id,
                  int page = -1, HFONT font = nullptr) {
    HWND child = CreateWindowExW(0, type, title, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, root,
        id ? reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)) : nullptr,
        GetModuleHandleW(nullptr), nullptr);
    if (child) {
        SendMessageW(child, WM_SETFONT,
            reinterpret_cast<WPARAM>(font ? font : g_font_regular), TRUE);
        if (page == 0) g_dial_controls.push_back(child);
        if (page == 1) g_settings_controls.push_back(child);
    }
    return child;
}
HWND make_label(HWND root, LPCWSTR text, int x, int y, int w, int h,
                int page = -1, HFONT font = nullptr) {
    return make_control(root, L"STATIC", text, SS_LEFT | SS_NOPREFIX,
                        x, y, w, h, 0, page, font);
}
HWND make_button(HWND root, LPCWSTR text, int id, int x, int y, int w, int h,
                 int page = -1, HFONT font = nullptr) {
    return make_control(root, L"BUTTON", text, WS_TABSTOP | BS_OWNERDRAW,
                        x, y, w, h, id, page, font);
}
HWND make_edit(HWND root, LPCWSTR initial, int id, int x, int y, int w, int h,
               int page) {
    HWND edit = make_control(root, L"EDIT", initial,
        WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, x, y, w, h, id, page);
    SendMessageW(edit, EM_LIMITTEXT, 128, 0);
    return edit;
}
void show_page(HWND root, int page) {
    g_page = page;
    for (auto child : g_dial_controls) ShowWindow(child, page == 0 ? SW_SHOW : SW_HIDE);
    for (auto child : g_settings_controls) ShowWindow(child, page == 1 ? SW_SHOW : SW_HIDE);
    InvalidateRect(root, nullptr, TRUE);
    HWND target = page == 0 ? g_number : g_server;
    if (target) SetFocus(target);
}
void draw_card(HDC dc, RECT rect, COLORREF color, int radius) {
    HBRUSH fill = CreateSolidBrush(color);
    HPEN border = CreatePen(PS_SOLID, 1, line_color());
    HGDIOBJ old_brush = SelectObject(dc, fill);
    HGDIOBJ old_pen = SelectObject(dc, border);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(border);
    DeleteObject(fill);
}
void paint_button(const DRAWITEMSTRUCT& item) {
    const int id = static_cast<int>(item.CtlID);
    const bool disabled = (item.itemState & ODS_DISABLED) != 0;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool selected_tab = (id == kShowDial && g_page == 0) ||
                              (id == kShowSettings && g_page == 1);
    COLORREF color = panel_color();
    if (selected_tab || id == kCallButton) color = accent_color();
    if (id >= kDigitStart && id < kDigitStart + 12) color = field_color();
    if (pressed) color = g_dark ? RGB(44, 91, 111) : RGB(206, 227, 239);
    if (disabled) color = g_dark ? RGB(42, 53, 70) : RGB(222, 228, 235);

    RECT rect = item.rcItem;
    HBRUSH fill = CreateSolidBrush(color);
    HPEN outline = CreatePen(PS_SOLID, 1, selected_tab ? accent_color() : line_color());
    HGDIOBJ brush_old = SelectObject(item.hDC, fill);
    HGDIOBJ pen_old = SelectObject(item.hDC, outline);
    RoundRect(item.hDC, rect.left + 1, rect.top + 1,
              rect.right - 1, rect.bottom - 1, 13, 13);
    SelectObject(item.hDC, pen_old);
    SelectObject(item.hDC, brush_old);
    DeleteObject(outline);
    DeleteObject(fill);

    const COLORREF foreground = disabled ? muted_color()
        : (selected_tab || id == kCallButton ? RGB(5, 25, 38) : text_color());
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, foreground);
    wchar_t title[96] = {};
    GetWindowTextW(item.hwndItem, title, 95);

    if (id >= kDigitStart && id < kDigitStart + 12) {
        const auto index = static_cast<size_t>(id - kDigitStart);
        const auto digit = axv::dialpad_keys()[index];
        const wchar_t primary[2] = {digit, 0};
        RECT number = rect;
        number.bottom = number.top + 37;
        RECT subtitle = rect;
        subtitle.top += 36;
        auto old = SelectObject(item.hDC, g_font_digit);
        DrawTextW(item.hDC, primary, -1, &number, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        SelectObject(item.hDC, g_font_small);
        const wchar_t* letters[] = {
            L"", L"ABC", L"DEF", L"GHI", L"JKL", L"MNO",
            L"PQRS", L"TUV", L"WXYZ", L"", L"", L""
        };
        SetTextColor(item.hDC, muted_color());
        DrawTextW(item.hDC, letters[index], -1, &subtitle, DT_CENTER | DT_SINGLELINE);
        SelectObject(item.hDC, old);
    } else {
        HGDIOBJ old_font = SelectObject(item.hDC, g_font_regular);
        DrawTextW(item.hDC, title, -1, &rect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(item.hDC, old_font);
    }
    if (item.itemState & ODS_FOCUS) {
        InflateRect(&rect, -5, -5);
        DrawFocusRect(item.hDC, &rect);
    }
}

void edit_number(HWND root, wchar_t action) {
    wchar_t buffer[129] = {};
    GetWindowTextW(g_number, buffer, 129);
    std::wstring candidate(buffer);
    if (action == L'\b') axv::dial_backspace(candidate);
    else if (action == L'\0') candidate.clear();
    else axv::dial_append(candidate, action);
    SetWindowTextW(g_number, candidate.c_str());
    SendMessageW(g_number, EM_SETSEL, candidate.length(), candidate.length());
    SetFocus(g_number);
    if (candidate.empty()) {
        SetWindowTextW(g_status, L"Digite um ramal ou endereço SIP.");
    } else {
        SetWindowTextW(g_status, L"Destino preparado. Chamadas reais ainda não habilitadas.");
    }
    (void)root;
}

void begin_shutdown(HWND window) {
    const bool queued = g_baresip_process.request_quit_async();
    g_shutdown.begin(GetTickCount64());
    EnableWindow(GetDlgItem(window, kStartBaresipButton), FALSE);
    EnableWindow(GetDlgItem(window, kStopBaresipButton), FALSE);
    SetWindowTextW(g_process_status, queued
        ? L"Pedido de /quit agendado; aguardando a saída do processo."
        : L"Não foi possível agendar /quit; encerramento forçado ao atingir o prazo.");
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        load_ui_preferences();
        g_font_regular = create_font(17, FW_MEDIUM);
        g_font_title = create_font(27, FW_BOLD);
        g_font_small = create_font(13, FW_NORMAL);
        g_font_digit = create_font(25, FW_SEMIBOLD);
        refresh_brushes();
        make_label(window, L"AXE  VOICE", 24, 16, 240, 37, -1, g_font_title);
        make_label(window, L"SOFTPHONE DESKTOP  /  LAB", 25, 57, 270, 22, -1, g_font_small);
        make_button(window, g_pinned ? L"SOLTAR" : L"FIXAR", kPin, 265, 18, 70, 36);
        make_button(window, g_dark ? L"CLARO" : L"ESCURO", kTheme, 344, 18, 70, 36);
        if (g_pinned) SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
                                   SWP_NOMOVE | SWP_NOSIZE);
        make_button(window, L"TECLADO", kShowDial, 24, 103, 186, 42);
        make_button(window, L"AJUSTES", kShowSettings, 216, 103, 198, 42);
        make_label(window, L"●  CONTA SIP NÃO REGISTRADA", 42, 164, 347, 25, -1, g_font_regular);

        make_label(window, L"DISCAGEM", 39, 220, 290, 23, 0, g_font_small);
        make_label(window, L"Número ou endereço SIP", 39, 246, 320, 20, 0, g_font_regular);
        g_number = make_edit(window, L"", kNumberEdit, 39, 272, 350, 43, 0);

        constexpr const wchar_t* dial_labels[] = {
            L"1",L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"*",L"0",L"#"
        };
        for (int i = 0; i != 12; ++i) {
            const int col = i % 3;
            const int row = i / 3;
            make_button(window, dial_labels[i], kDigitStart + i,
                        39 + col * 119, 329 + row * 66, 111, 59, 0);
        }
        make_button(window, L"APAGAR", kBackspace, 39, 597, 170, 38, 0);
        make_button(window, L"LIMPAR", kClear, 218, 597, 171, 38, 0);
        make_button(window, L"CHAMAR  •  SIP INDISPONÍVEL", kCallButton,
                    39, 651, 350, 51, 0);
        make_button(window, L"MUDO", kMuteFuture, 39, 708, 82, 34, 0);
        make_button(window, L"ESPERA", kHoldFuture, 127, 708, 82, 34, 0);
        make_button(window, L"TRANSF.", kTransferFuture, 215, 708, 82, 34, 0);
        make_button(window, L"ENCERRAR", kHangupFuture, 303, 708, 86, 34, 0);

        make_label(window, L"CONFIGURAÇÃO DE CONTA  •  SEM REGISTRO", 39, 219, 361, 26,
                   1, g_font_small);
        make_label(window, L"Servidor SIP", 39, 255, 340, 20, 1);
        g_server = make_edit(window, L"", kServerEdit, 39, 280, 350, 38, 1);
        make_label(window, L"Usuário SIP", 39, 329, 212, 20, 1);
        make_label(window, L"Porta", 268, 329, 118, 20, 1);
        g_user = make_edit(window, L"", kUserEdit, 39, 355, 216, 38, 1);
        g_port = make_edit(window, L"5060", kPortEdit, 267, 355, 122, 38, 1);
        g_tls = make_control(window, L"BUTTON", L"Usar TLS (quando suportado)",
                             WS_TABSTOP | BS_AUTOCHECKBOX, 39, 404, 340, 28,
                             kTlsCheckbox, 1);
        make_button(window, L"VALIDAR DADOS  /  SEM REGISTRAR", kValidateAccount,
                    39, 440, 350, 38, 1);
        make_label(window, L"DIAGNÓSTICOS LOCAIS", 39, 492, 331, 19, 1, g_font_small);
        make_button(window, L"DISPOSITIVOS DE ÁUDIO", kAudioDevicesButton,
                    39, 517, 350, 36, 1);
        make_button(window, L"VERIFICAR BARESIP", kProbeButton,
                    39, 561, 350, 36, 1);
        make_button(window, L"INICIAR MOTOR", kStartBaresipButton,
                    39, 605, 170, 39, 1);
        make_button(window, L"PARAR MOTOR", kStopBaresipButton,
                    218, 605, 171, 39, 1);
        g_audio_status = make_label(window, L"Áudio não inspecionado.",
                                    39, 654, 350, 41, 1, g_font_small);
        g_process_status = make_label(window, L"Baresip parado; registro SIP indisponível.",
                                      39, 702, 350, 43, 1, g_font_small);
        g_status = make_label(window, L"Digite um ramal ou endereço SIP.",
                              24, 762, 390, 30, -1, g_font_small);

        // A clear disabled affordance: no real SIP engine has been connected yet.
        for (int id : {kCallButton, kMuteFuture, kHoldFuture,
                       kTransferFuture, kHangupFuture}) {
            EnableWindow(GetDlgItem(window, id), FALSE);
        }
        show_page(window, 0);
        SetTimer(window, kProcessTimer, 100, nullptr);
        return 0;
    }
    case WM_COMMAND:
        // BN_CLICKED comes from buttons (0); accelerator-generated commands use 1.
        if (HIWORD(wparam) == BN_CLICKED || HIWORD(wparam) == 1) {
            const int control = LOWORD(wparam);
            if (control == kShowDial || control == kFocusNumber) { show_page(window, 0); return 0; }
            if (control == kShowSettings) { show_page(window, 1); return 0; }
            if (control == kTheme) {
                g_dark = !g_dark;
                SetWindowTextW(GetDlgItem(window, kTheme), g_dark ? L"CLARO" : L"ESCURO");
                refresh_brushes();
                save_ui_preferences();
                InvalidateRect(window, nullptr, TRUE);
                EnumChildWindows(window, repaint_child, 0);
                return 0;
            }
            if (control == kPin) {
                g_pinned = !g_pinned;
                SetWindowPos(window, g_pinned ? HWND_TOPMOST : HWND_NOTOPMOST,
                             0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetWindowTextW(GetDlgItem(window, kPin), g_pinned ? L"SOLTAR" : L"FIXAR");
                save_ui_preferences();
                return 0;
            }
            if (control >= kDigitStart && control < kDigitStart + 12) {
                edit_number(window, axv::dialpad_keys()[control - kDigitStart]);
                return 0;
            }
            if (control == kBackspace) { edit_number(window, L'\b'); return 0; }
            if (control == kClear) { edit_number(window, L'\0'); return 0; }
        }
        if (LOWORD(wparam) == kStartBaresipButton && HIWORD(wparam) == BN_CLICKED) {
            if (g_shutdown.pending()) {
                SetWindowTextW(g_process_status, L"Aguarde a parada do motor.");
                return 0;
            }
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
            const auto modules = axv::inspect_baresip_module_files(candidate);
            if (modules.status == axv::BaresipModuleScanStatus::UnsafeBinary ||
                modules.status == axv::BaresipModuleScanStatus::UnsafeModuleDirectory) {
                const auto diagnostic = axv::baresip_module_inventory_summary(modules);
                SetWindowTextW(g_process_status, diagnostic.c_str());
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
            PWSTR local_app_data = nullptr;
            const HRESULT location_result =
                SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &local_app_data);
            if (FAILED(location_result) || !local_app_data) {
                if (local_app_data) CoTaskMemFree(local_app_data);
                SetWindowTextW(g_process_status, L"Não foi possível encontrar o perfil local do Windows.");
                return 0;
            }
            const std::filesystem::path profile =
                std::filesystem::path(local_app_data) / L"AXEVoice" / L"baresip-lab";
            CoTaskMemFree(local_app_data);
            const auto profile_status = axv::prepare_baresip_lab_profile(profile);
            if (profile_status != axv::BaresipProfileStatus::Ready) {
                SetWindowTextW(g_process_status, axv::baresip_profile_diagnostic(profile_status));
                return 0;
            }
            if (g_baresip_process.start(candidate, profile)) {
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
            if (g_shutdown.pending()) return 0;
            if (g_baresip_process.poll() != axv::ProcessStatus::Running) {
                SetWindowTextW(g_process_status, L"Motor ja estava parado.");
                return 0;
            }
            begin_shutdown(window);
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
            const auto inventory = axv::inspect_baresip_module_files(candidate);
            const auto summary = axv::baresip_module_inventory_summary(inventory);
            MessageBoxW(window, summary.c_str(), L"Auditoria de modulos Baresip", MB_OK | MB_ICONINFORMATION);
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
    case WM_SIZE:
        if (wparam == SIZE_MINIMIZED && install_tray(window)) {
            ShowWindow(window, SW_HIDE);
            return 0;
        }
        break;
    case kTrayCallback:
        if (lparam == WM_LBUTTONDBLCLK) {
            restore_tray(window);
            return 0;
        }
        if (lparam == WM_RBUTTONUP || lparam == WM_CONTEXTMENU) {
            POINT location = {};
            GetCursorPos(&location);
            HMENU menu = CreatePopupMenu();
            if (menu) {
                AppendMenuW(menu, MF_STRING, kTrayOpen, L"Abrir AXE Voice");
                AppendMenuW(menu, MF_STRING, kTrayExit, L"Sair do AXE Voice");
                SetForegroundWindow(window);
                const auto choice = TrackPopupMenu(menu,
                    TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON,
                    location.x, location.y, 0, window, nullptr);
                DestroyMenu(menu);
                if (choice == kTrayOpen) restore_tray(window);
                if (choice == kTrayExit) SendMessageW(window, WM_CLOSE, 0, 0);
                PostMessageW(window, WM_NULL, 0, 0);
            }
            return 0;
        }
        break;
    case WM_ERASEBKGND: {
        RECT rect = {};
        GetClientRect(window, &rect);
        FillRect(reinterpret_cast<HDC>(wparam), &rect, g_background_brush);
        return 1;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        SetTextColor(dc, text_color());
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        SetTextColor(dc, text_color());
        SetBkColor(dc, field_color());
        return reinterpret_cast<LRESULT>(g_field_brush);
    }
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        SetTextColor(dc, text_color());
        SetBkColor(dc, background_color());
        return reinterpret_cast<LRESULT>(g_background_brush);
    }
    case WM_DRAWITEM: {
        const auto* item = reinterpret_cast<const DRAWITEMSTRUCT*>(lparam);
        if (item && item->CtlType == ODT_BUTTON) {
            paint_button(*item);
            return TRUE;
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(window, &paint);
        RECT rect = {};
        GetClientRect(window, &rect);
        FillRect(dc, &rect, g_background_brush);
        draw_card(dc, {24, 151, 414, 198}, panel_color(), 16);
        draw_card(dc, {24, 206, 414, 742}, panel_color(), 18);
        HPEN rule = CreatePen(PS_SOLID, 1, line_color());
        HGDIOBJ old = SelectObject(dc, rule);
        MoveToEx(dc, 24, 747, nullptr); LineTo(dc, 414, 747);
        SelectObject(dc, old);
        DeleteObject(rule);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_TIMER:
        if (wparam == kProcessTimer) {
            if (g_shutdown.pending()) {
                const auto state = g_baresip_process.poll();
                if (state == axv::ProcessStatus::Exited) {
                    g_shutdown.clear();
                    g_last_process_state = axv::ProcessStatus::Stopped;
                    SetWindowTextW(g_process_status,
                        L"Processo encerrou apos pedido de /quit; SIP nao homologado.");
                } else if (state != axv::ProcessStatus::Running ||
                           g_shutdown.expired(GetTickCount64(), kGracePeriodMs)) {
                    const bool stopped = g_baresip_process.stop();
                    g_shutdown.clear();
                    g_last_process_state = stopped ? axv::ProcessStatus::Stopped
                                                   : axv::ProcessStatus::Failed;
                    SetWindowTextW(g_process_status, stopped
                        ? L"Timeout ou falha de processo: encerramento forcado pelo Job Object."
                        : L"Nao foi possivel finalizar o processo; verificar tarefas do Windows.");
                } else {
                    return 0;
                }
                if (g_close_requested) {
                    DestroyWindow(window); // Graceful exit or forced fallback completed.
                    return 0;
                }
                EnableWindow(GetDlgItem(window, kStartBaresipButton), TRUE);
                EnableWindow(GetDlgItem(window, kStopBaresipButton), TRUE);
                return 0;
            }
            const auto state = g_baresip_process.poll();
            if (state != g_last_process_state) {
                g_last_process_state = state;
                if (state == axv::ProcessStatus::Exited) {
                    const auto code = g_baresip_process.exit_code();
                    const std::wstring result_text = code
                        ? L"Baresip encerrou (codigo " + std::to_wstring(*code) +
                          L"). Registro SIP nao confirmado."
                        : L"Baresip encerrou; codigo indisponivel. Registro nao confirmado.";
                    SetWindowTextW(g_process_status, result_text.c_str());
                } else if (state == axv::ProcessStatus::Failed) {
                    SetWindowTextW(g_process_status, L"Falha ao consultar processo Baresip.");
                }
            }
            return 0;
        }
        break;
    case WM_CLOSE:
        if (g_shutdown.pending()) {
            g_close_requested = true;
            return 0;
        }
        if (g_baresip_process.poll() == axv::ProcessStatus::Running) {
            g_close_requested = true;
            begin_shutdown(window);
            return 0;
        }
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, kProcessTimer);
        remove_tray(window);
        if (g_font_regular) DeleteObject(g_font_regular);
        if (g_font_title) DeleteObject(g_font_title);
        if (g_font_small) DeleteObject(g_font_small);
        if (g_font_digit) DeleteObject(g_font_digit);
        if (g_background_brush) DeleteObject(g_background_brush);
        if (g_field_brush) DeleteObject(g_field_brush);
        // Normally stopped through WM_CLOSE. Forced fallback on external destruction.
        g_baresip_process.stop();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command_line, int show) {
    const wchar_t* className = L"AXEVoiceDesktopWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc)) return 1;
    HWND window = CreateWindowExW(0, className, L"AXE Voice  |  Desktop Lab v0.21",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 438, 842,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 2;
    if (command_line && std::wcscmp(command_line, L"--ui-smoke") == 0) {
        // Headless GUI startup check for Windows CI: verify the real controls
        // and both pages without contacting any SIP server or persisting data.
        bool ok = GetDlgItem(window, kShowDial) && GetDlgItem(window, kShowSettings) &&
                  GetDlgItem(window, kNumberEdit) && GetDlgItem(window, kCallButton) &&
                  GetDlgItem(window, kServerEdit) && GetDlgItem(window, kAudioDevicesButton) &&
                  !IsWindowEnabled(GetDlgItem(window, kCallButton));
        for (int i = 0; i != 12; ++i)
            ok = ok && GetDlgItem(window, kDigitStart + i) != nullptr;
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kShowSettings, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kShowSettings)));
        ok = ok && (GetWindowLongW(g_server, GWL_STYLE) & WS_VISIBLE) &&
             !(GetWindowLongW(g_number, GWL_STYLE) & WS_VISIBLE);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kShowDial, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kShowDial)));
        ok = ok && (GetWindowLongW(g_number, GWL_STYLE) & WS_VISIBLE) &&
             !(GetWindowLongW(g_server, GWL_STYLE) & WS_VISIBLE);
        DestroyWindow(window);
        return ok ? 0 : 9;
    }
    ShowWindow(window, show);
    UpdateWindow(window);
    const ACCEL shortcuts[] = {
        {FVIRTKEY | FCONTROL, L'K', kShowDial},
        {FVIRTKEY | FCONTROL, L'L', kFocusNumber},
        {FVIRTKEY | FCONTROL, VK_OEM_COMMA, kShowSettings},
        {FVIRTKEY | FCONTROL, L'T', kTheme},
        {FVIRTKEY | FCONTROL, L'P', kPin}
    };
    HACCEL accelerator = CreateAcceleratorTableW(
        const_cast<ACCEL*>(shortcuts), static_cast<int>(sizeof(shortcuts)/sizeof(shortcuts[0])));
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (accelerator && TranslateAcceleratorW(window, accelerator, &msg)) continue;
        if (IsDialogMessageW(window, &msg)) continue; // Tab / Shift+Tab among Win32 controls.
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (accelerator) DestroyAcceleratorTable(accelerator);
    return static_cast<int>(msg.wParam);
}
