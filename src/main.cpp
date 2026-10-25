#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commctrl.h>
#include <dbt.h>
#include "baresip_profile.h"
#include "sip_address.h"
#include "sip_engine.h"
#include "sip_account.h"
#include "sip_status.h"
#include "sip_runtime_observation.h"
#include "baresip_event_mailbox.h"
#include "sip_event_projection.h"
#include "sip_runtime_session_guard.h"
#include "incoming_ringtone_coordinator.h"
#include "dial_authorization.h"
#include "call_policy.h"
#include "sip_diagnostics.h"
#include "sip_registration_policy.h"
#include "sip_security_policy.h"
#include "sip_engine_factory.h"
#include "baresip_probe.h"
#include "baresip_modules.h"
#include "audio_devices.h"
#include "audio_selection_feedback.h"
#include "audio_hotplug_policy.h"
#include "audio_routing_preview.h"
#include "ringtone_preview.h"
#include "audio_role_swap.h"
#include "audio_preview_target.h"
#include "audio_preview_cancel_policy.h"
#include "audio_preview_levels.h"
#include "audio_route_change_fence.h"
#include "windows_process.h"
#include "shutdown_policy.h"
#include <string>
#include <string_view>
#include <thread>
#include <atomic>
#include <utility>
#include <cstdio>

#include "dialpad_model.h"
#include "destination_feedback.h"
#include "account_form_feedback.h"
#include "ui_help.h"
#include "call_readiness.h"
#include "readiness_export.h"
#include "ui_contrast_policy.h"
#include "ui_dpi_policy.h"
#include "ui_viewport_policy.h"
#include "desktop_extras_win32.h"
#include "sip_native_preflight.h"
#include "native_sip_release_gate.h"
#include "sip_vault_status_win32.h"
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
constexpr int kRefreshHeadsetsButton = 1013;
constexpr int kSwapAudioRolesButton = 1015;
constexpr int kStartBaresipButton = 1010;
constexpr int kStopBaresipButton = 1011;
constexpr UINT_PTR kProcessTimer = 1012;
constexpr int kShowDial = 2001;
constexpr int kShowSettings = 2002;
constexpr int kTheme = 2003;
constexpr int kPin = 2004;
constexpr int kCompact = 2005;
constexpr int kShowAudio = 2006;
constexpr int kHelpButton = 2007;
constexpr int kReadinessButton = 2008;
constexpr int kInputAudioCombo = 2401;
constexpr int kOutputAudioCombo = 2402;
constexpr int kRingtoneAudioCombo = 2403;
constexpr int kTestRingtoneButton = 2404;
constexpr int kTestVoiceButton = 2405;
constexpr int kVoicePreviewLevel = 2406;
constexpr int kRingPreviewLevel = 2407;
constexpr int kWindowWidth = 438;
unsigned g_dpi = axv::kBaseDpi;
constexpr int kFullHeight = 842;
constexpr int kFullScrollContentEnd = 804;
constexpr int kMiniScrollContentEnd = 340;
int g_scroll_offset = 0;
int g_scroll_max = 0;
int g_wheel_remainder = 0;
bool g_controls_ready = false;
bool g_scrollbar_updating = false;
constexpr int kCompactHeight = 365;
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
constexpr int kMoreButton = 2306;
constexpr int kSaveFavorite = 3101;
constexpr int kLoadFavorite = 3102;
constexpr int kShowRecent = 3103;
constexpr int kPrivacyDial = 3104;
constexpr int kCloseToTrayToggle = 3105;
constexpr int kAutostartToggle = 3106;
constexpr int kClearRecent = 3107;
constexpr int kExplicitExit = 3108;
constexpr int kRecentPickStart = 3200;
constexpr int kSipPreflight = 3114;
constexpr int kSipVaultStatus = 3115;
constexpr int kOpenCredentialManager = 3116;
constexpr int kRemoveSipVaultCredential = 3118;
constexpr int kSipNextSteps = 3117;
constexpr int kStartupInTrayToggle = 3110;
constexpr int kDeleteFavorite = 3111;
constexpr int kRecallLastDestination = 3112;
constexpr int kCopySafeDiagnostics = 3113;
constexpr int kCopyDestination = 3120;
constexpr int kPasteDestination = 3121;
constexpr int kSelectAllDestination = 3122;
constexpr int kUndoDialEdit = 3123;
constexpr int kResetDialWorkspace = 3124;
HWND g_number = nullptr;
HWND g_number_label = nullptr;
HWND g_status = nullptr;
HWND g_server = nullptr;
HWND g_user = nullptr;
HWND g_port = nullptr;
HWND g_tls = nullptr;
HWND g_audio_status = nullptr;
HWND g_process_status = nullptr;
HWND g_runtime_banner = nullptr;
HWND g_input_audio_combo = nullptr;
HWND g_output_audio_combo = nullptr;
HWND g_ringtone_audio_combo = nullptr;
HWND g_voice_level_combo = nullptr;
HWND g_ring_level_combo = nullptr;
HWND g_headset_status = nullptr;
std::vector<axv::AudioEndpoint> g_audio_inputs;
std::vector<axv::AudioEndpoint> g_audio_outputs;
std::wstring g_selected_input_id;
std::wstring g_selected_output_id;
std::wstring g_selected_ringtone_id;
// The explicitly chosen endpoints survive temporary enumeration failure or
// a temporary disconnect. Fallback endpoints never replace the preference.
std::wstring g_preferred_input_id;
std::wstring g_preferred_output_id;
std::wstring g_preferred_ringtone_id;
axv::AudioHotplugPolicy g_audio_hotplug;
std::thread g_preview_worker;
std::atomic<bool> g_preview_running{false};
std::atomic<bool> g_preview_cancel{false};
std::atomic<int> g_preview_result{0};
bool g_preview_pending = false;
axv::AudioPreviewRole g_preview_role = axv::AudioPreviewRole::Ringtone;
std::uint8_t g_voice_level = axv::kPreviewDefaultLevel;
std::uint8_t g_ring_level = axv::kPreviewDefaultLevel;
axv::WindowsProcess g_baresip_process;
axv::ProcessStatus g_last_process_state = axv::ProcessStatus::Stopped;
axv::ShutdownDeadline g_shutdown;
bool g_close_requested = false;
constexpr std::uint64_t kGracePeriodMs = 1500;
axv::EngineSelection g_sip_selection = axv::create_sip_engine();
// Prepared for future native callbacks; current laboratory build has no producer.
axv::BaresipEventMailbox g_native_event_mailbox;
axv::SipEventProjection g_sip_projection;
axv::SipRuntimeSessionGuard g_sip_session_guard;
axv::IncomingRingtoneCoordinator g_incoming_ring_coordinator;
axv::DisabledIncomingRingtoneSink g_incoming_ring_sink;
axv::AudioRouteChangeFence g_audio_route_fence;


bool g_dark = true;
bool g_high_contrast = false;
axv::UiSystemColors g_system_colors{};
bool g_pinned = false;
bool g_compact = false;
bool g_smoke_mode = false;
bool g_help_requested_smoke = false;
bool g_readiness_requested_smoke = false;
bool g_destination_invalid = false;
bool g_account_invalid_server = false;
bool g_account_invalid_user = false;
bool g_account_invalid_port = false;
bool g_tray_visible = false;
bool g_close_to_tray = false;
bool g_explicit_exit = false;
bool g_private_dial = false;
std::wstring g_favorite_destination;
std::vector<std::wstring> g_recent_destinations;
constexpr wchar_t kPrefsKey[] = L"Software\\AXEVoice\\Desktop";
int g_page = 0; // 0 = dial, 1 = settings, 2 = audio
std::vector<HWND> g_dial_controls;
std::vector<HWND> g_settings_controls;
std::vector<HWND> g_audio_controls;
enum class UiFontKind { Regular, Title, Small, Digit };
struct ChildLayout { HWND handle; axv::DpiBox logical; UiFontKind font; };
std::vector<ChildLayout> g_layout;
HFONT g_font_regular = nullptr;
HFONT g_font_title = nullptr;
HFONT g_font_small = nullptr;
HFONT g_font_digit = nullptr;
HBRUSH g_background_brush = nullptr;
HBRUSH g_field_brush = nullptr;
HBRUSH g_invalid_field_brush = nullptr;


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
    value = 0; size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kPrefsKey, L"CompactMode", RRF_RT_REG_DWORD,
                     nullptr, &value, &size) == ERROR_SUCCESS) {
        g_compact = value != 0;
    }
    value = 0; size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kPrefsKey, L"CloseToTray", RRF_RT_REG_DWORD,
                     nullptr, &value, &size) == ERROR_SUCCESS) {
        g_close_to_tray = value != 0;
    }
    g_favorite_destination = axv::load_desktop_favorite(kPrefsKey);
}
void save_ui_preferences() {
    if (g_smoke_mode) return; // Smoke may not write to the user's registry.
    HKEY key = nullptr;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kPrefsKey, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, &disposition) != ERROR_SUCCESS) return;
    const DWORD theme = g_dark ? 1u : 0u;
    const DWORD pinned = g_pinned ? 1u : 0u;
    const DWORD compact = g_compact ? 1u : 0u;
    const DWORD close_to_tray = g_close_to_tray ? 1u : 0u;
    RegSetValueExW(key, L"CompactMode", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&compact), sizeof(compact));
    RegSetValueExW(key, L"DarkTheme", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&theme), sizeof(theme));
    RegSetValueExW(key, L"AlwaysOnTop", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&pinned), sizeof(pinned));
    RegSetValueExW(key, L"CloseToTray", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&close_to_tray), sizeof(close_to_tray));
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
void read_windows_contrast() {
    HIGHCONTRASTW info{};
    info.cbSize = sizeof(info);
    const bool available = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(info),
                                                  &info, 0) != FALSE;
    g_high_contrast = available && (info.dwFlags & HCF_HIGHCONTRASTON) != 0;
    g_system_colors = axv::UiSystemColors{
        static_cast<axv::UiColor>(GetSysColor(COLOR_WINDOW)),
        static_cast<axv::UiColor>(GetSysColor(COLOR_WINDOWTEXT)),
        static_cast<axv::UiColor>(GetSysColor(COLOR_BTNFACE)),
        static_cast<axv::UiColor>(GetSysColor(COLOR_BTNTEXT)),
        static_cast<axv::UiColor>(GetSysColor(COLOR_HIGHLIGHT)),
        static_cast<axv::UiColor>(GetSysColor(COLOR_HIGHLIGHTTEXT)),
        static_cast<axv::UiColor>(GetSysColor(COLOR_GRAYTEXT))
    };
}
COLORREF ui_color(axv::UiRole role) {
    return static_cast<COLORREF>(axv::theme_color(role, g_dark,
                                                  g_high_contrast, g_system_colors));
}
COLORREF background_color() { return ui_color(axv::UiRole::Background); }
COLORREF panel_color() { return ui_color(axv::UiRole::Panel); }
COLORREF field_color() { return ui_color(axv::UiRole::Field); }
COLORREF invalid_field_color() { return ui_color(axv::UiRole::InvalidField); }
COLORREF text_color() { return ui_color(axv::UiRole::Text); }
COLORREF muted_color() { return ui_color(axv::UiRole::Muted); }
COLORREF accent_color() { return ui_color(axv::UiRole::Accent); }
COLORREF line_color() { return ui_color(axv::UiRole::Border); }

void refresh_brushes() {
    if (g_background_brush) DeleteObject(g_background_brush);
    if (g_field_brush) DeleteObject(g_field_brush);
    if (g_invalid_field_brush) DeleteObject(g_invalid_field_brush);
    g_background_brush = CreateSolidBrush(background_color());
    g_field_brush = CreateSolidBrush(field_color());
    g_invalid_field_brush = CreateSolidBrush(invalid_field_color());
}
HFONT create_font(int size, int weight) {
    return CreateFontW(-axv::dpi_pixels(size, g_dpi), 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

HFONT font_for_kind(UiFontKind kind) {
    switch(kind) {
    case UiFontKind::Title: return g_font_title;
    case UiFontKind::Small: return g_font_small;
    case UiFontKind::Digit: return g_font_digit;
    case UiFontKind::Regular: return g_font_regular;
    }
    return g_font_regular;
}
void rebuild_fonts() {
    HFONT next[] = {create_font(17,FW_MEDIUM), create_font(27,FW_BOLD),
                    create_font(13,FW_NORMAL), create_font(25,FW_SEMIBOLD)};
    for(HFONT font : next) if(!font) {
        for(HFONT allocated : next) if(allocated) DeleteObject(allocated);
        return;
    }
    HFONT prior[] = {g_font_regular, g_font_title, g_font_small, g_font_digit};
    g_font_regular=next[0]; g_font_title=next[1];
    g_font_small=next[2]; g_font_digit=next[3];
    for(const auto& child : g_layout)
        SendMessageW(child.handle, WM_SETFONT,
                     reinterpret_cast<WPARAM>(font_for_kind(child.font)), FALSE);
    for(HFONT old : prior) if(old) DeleteObject(old);
}
void position_logical(HWND child, int x,int y,int width,int height) {
    if(!child) return;
    const auto box = axv::scaled_box({x,y,width,height},g_dpi);
    SetWindowPos(child,nullptr,box.x,box.y-g_scroll_offset,box.width,box.height,
                 SWP_NOACTIVATE|SWP_NOZORDER);
}
RECT logical_rect(int x,int y,int right,int bottom) {
    return {axv::dpi_pixels(x,g_dpi), axv::dpi_pixels(y,g_dpi)-g_scroll_offset,
            axv::dpi_pixels(right,g_dpi), axv::dpi_pixels(bottom,g_dpi)-g_scroll_offset};
}
HWND make_control(HWND root, LPCWSTR type, LPCWSTR title, DWORD style,
                  int x, int y, int width, int height, int id,
                  int page = -1, HFONT font = nullptr) {
    const auto box = axv::scaled_box({x,y,width,height},g_dpi);
    HWND child = CreateWindowExW(0, type, title, WS_CHILD | WS_VISIBLE | style,
        box.x, box.y-g_scroll_offset, box.width, box.height, root,
        id ? reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)) : nullptr,
        GetModuleHandleW(nullptr), nullptr);
    if (child) {
        const UiFontKind kind = font == g_font_title ? UiFontKind::Title :
            font == g_font_small ? UiFontKind::Small :
            font == g_font_digit ? UiFontKind::Digit : UiFontKind::Regular;
        g_layout.push_back({child,{x,y,width,height},kind});
        SendMessageW(child, WM_SETFONT,
            reinterpret_cast<WPARAM>(font ? font : g_font_regular), TRUE);
        if (page == 0) g_dial_controls.push_back(child);
        if (page == 1) g_settings_controls.push_back(child);
        if (page == 2) g_audio_controls.push_back(child);
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
    if (edit) {
        SendMessageW(edit, EM_LIMITTEXT, 128, 0);
        const wchar_t* hint = nullptr;
        switch (id) {
        case kNumberEdit: hint = L"Ex.: 1001 ou sip:usuario@dominio"; break;
        case kServerEdit: hint = L"pbx.exemplo.com"; break;
        case kUserEdit: hint = L"Ex.: 1001"; break;
        case kPortEdit: hint = L"5060"; break;
        default: break;
        }
        if (hint) SendMessageW(edit, EM_SETCUEBANNER, TRUE,
                               reinterpret_cast<LPARAM>(hint));
    }
    return edit;
}
void refresh_destination_feedback();
void refresh_account_feedback() {
    if (!g_server || !g_user || !g_port || !g_status) return;
    wchar_t server[129] = {}, username[129] = {}, port_text[129] = {};
    GetWindowTextW(g_server, server, 129);
    GetWindowTextW(g_user, username, 129);
    GetWindowTextW(g_port, port_text, 129);
    const auto feedback = axv::evaluate_account_form(server, username, port_text);
    g_account_invalid_server = feedback.kind == axv::AccountFeedbackKind::Invalid &&
                               feedback.error == axv::AccountError::InvalidServer;
    g_account_invalid_user = feedback.kind == axv::AccountFeedbackKind::Invalid &&
                             feedback.error == axv::AccountError::InvalidUser;
    g_account_invalid_port = feedback.kind == axv::AccountFeedbackKind::Invalid &&
                             feedback.error == axv::AccountError::InvalidPort;
    if (g_page == 1) SetWindowTextW(g_status, feedback.message);
    InvalidateRect(g_server, nullptr, TRUE);
    InvalidateRect(g_user, nullptr, TRUE);
    InvalidateRect(g_port, nullptr, TRUE);
}
// Viewport only changes UI coordinates; it never authorizes calls.
void position_all_children() {
    for (const auto& child : g_layout) {
        auto r=child.logical;
        if (child.handle==g_number_label) r.y=g_compact?218:246;
        if (child.handle==g_number) r.y=g_compact?242:272;
        if (child.handle==g_status) r.y=g_compact?298:762;
        position_logical(child.handle,r.x,r.y,r.width,r.height);
    }
}
int viewport_content_height() {
    return axv::dpi_pixels(g_compact?kMiniScrollContentEnd:kFullScrollContentEnd,g_dpi);
}
void sync_viewport_scroll(HWND window) {
    if (!g_controls_ready || g_scrollbar_updating) return;
    g_scrollbar_updating=true;
    RECT client{};
    GetClientRect(window,&client);
    const int visible=client.bottom-client.top;
    const int content=viewport_content_height();
    const int previous=g_scroll_offset;
    g_scroll_max=axv::max_scroll_offset(content,visible);
    g_scroll_offset=axv::clamp_scroll_offset(g_scroll_offset,content,visible);
    SCROLLINFO info{};
    info.cbSize=sizeof(info);
    info.fMask=SIF_RANGE|SIF_PAGE|SIF_POS;
    info.nMin=0;
    info.nMax=content>0?content-1:0;
    info.nPage=static_cast<UINT>(std::max(0,visible));
    info.nPos=g_scroll_offset;
    SetScrollInfo(window,SB_VERT,&info,TRUE);
    ShowScrollBar(window,SB_VERT,g_scroll_max>0);
    if (previous!=g_scroll_offset) {
        position_all_children();
        InvalidateRect(window,nullptr,TRUE);
    }
    g_scrollbar_updating=false;
}
void set_viewport_scroll(HWND window,int desired) {
    if (!g_controls_ready) return;
    RECT client{};
    GetClientRect(window,&client);
    const int target=axv::clamp_scroll_offset(desired,
        viewport_content_height(),client.bottom-client.top);
    if (target==g_scroll_offset) return;
    g_scroll_offset=target;
    sync_viewport_scroll(window);
    position_all_children();
    InvalidateRect(window,nullptr,TRUE);
}
void bring_focused_control_into_view(HWND window,HWND control) {
    if (!g_controls_ready || !control) return;
    for (const auto& child:g_layout) {
        if (child.handle!=control) continue;
        int y=child.logical.y;
        if (control==g_number_label) y=g_compact?218:246;
        if (control==g_number) y=g_compact?242:272;
        if (control==g_status) y=g_compact?298:762;
        RECT client{};
        GetClientRect(window,&client);
        const int top=axv::dpi_pixels(y,g_dpi);
        const int bottom=top+axv::dpi_pixels(child.logical.height,g_dpi);
        const int target=axv::scroll_for_visible_range(
            g_scroll_offset,viewport_content_height(),client.bottom-client.top,
            top,bottom,axv::dpi_pixels(8,g_dpi));
        set_viewport_scroll(window,target);
        break;
    }
}

void show_page(HWND root, int page) {
    if (g_page != page) set_viewport_scroll(root,0);
    if (page != 2 && g_preview_running.load()) g_preview_cancel.store(true);
    g_page = page;
    for (auto child : g_dial_controls) {
        const bool visible = page == 0 &&
            (!g_compact || child == g_number || child == g_number_label);
        ShowWindow(child, visible ? SW_SHOW : SW_HIDE);
    }
    for (auto child : g_settings_controls) ShowWindow(child, page == 1 ? SW_SHOW : SW_HIDE);
    for (auto child : g_audio_controls) ShowWindow(child, page == 2 ? SW_SHOW : SW_HIDE);
    sync_viewport_scroll(root);
    InvalidateRect(root, nullptr, TRUE);
    HWND target = page == 0 ? g_number : (page == 1 ? g_server : g_input_audio_combo);
    if (target) SetFocus(target);
    if (page == 0) refresh_destination_feedback();
    else if (page == 1) refresh_account_feedback();
    else if (g_status) SetWindowTextW(g_status,
        L"Dispositivos inspecionados localmente. Sem audio SIP ativo.");
}

// Mini mode keeps only the target and true SIP state, without fake call actions.
void apply_compact_layout(HWND root) {
    if (g_compact && g_page != 0) g_page = 0;
    position_logical(g_number_label,39,g_compact?218:246,320,20);
    position_logical(g_number,39,g_compact?242:272,350,43);
    position_logical(g_status,24,g_compact?298:762,390,30);
    SetWindowTextW(GetDlgItem(root, kCompact), g_compact ? L"MAX" : L"MINI");
    const int desired_height=axv::dpi_pixels(g_compact?kCompactHeight:kFullHeight,g_dpi);
    int fitted_height=desired_height;
    MONITORINFO monitor{};
    monitor.cbSize=sizeof(monitor);
    const HMONITOR display=MonitorFromWindow(root,MONITOR_DEFAULTTONEAREST);
    if (display && GetMonitorInfoW(display,&monitor)) {
        const int work_height=monitor.rcWork.bottom-monitor.rcWork.top;
        if (work_height>0) fitted_height=std::min(desired_height,work_height);
    }
    const int width=axv::dpi_pixels(kWindowWidth,g_dpi)+
                    GetSystemMetricsForDpi(SM_CXVSCROLL,g_dpi);
    SetWindowPos(root,nullptr,0,0,width,fitted_height,
                 SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    show_page(root,g_page);
    sync_viewport_scroll(root);
}
void draw_card(HDC dc, RECT rect, COLORREF color, int radius) {
    HBRUSH fill = CreateSolidBrush(color);
    HPEN border = CreatePen(PS_SOLID, 1, line_color());
    HGDIOBJ old_brush = SelectObject(dc, fill);
    HGDIOBJ old_pen = SelectObject(dc, border);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom,
              axv::dpi_pixels(radius,g_dpi),axv::dpi_pixels(radius,g_dpi));
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
                              (id == kShowSettings && g_page == 1) ||
                              (id == kShowAudio && g_page == 2);
    COLORREF color = ui_color(axv::UiRole::Button);
    if (selected_tab || id == kCallButton) color = accent_color();
    if (id >= kDigitStart && id < kDigitStart + 12) color = field_color();
    if (pressed) color = ui_color(axv::UiRole::Pressed);
    if (disabled) color = ui_color(axv::UiRole::DisabledButton);

    RECT rect = item.rcItem;
    HBRUSH fill = CreateSolidBrush(color);
    HPEN outline = CreatePen(PS_SOLID, 1, selected_tab ? accent_color() : line_color());
    HGDIOBJ brush_old = SelectObject(item.hDC, fill);
    HGDIOBJ pen_old = SelectObject(item.hDC, outline);
    const int offset=axv::dpi_pixels(1,g_dpi);
    const int curve=axv::dpi_pixels(13,g_dpi);
    RoundRect(item.hDC,rect.left+offset,rect.top+offset,
              rect.right-offset,rect.bottom-offset,curve,curve);
    SelectObject(item.hDC, pen_old);
    SelectObject(item.hDC, brush_old);
    DeleteObject(outline);
    DeleteObject(fill);

    const COLORREF foreground = disabled ? ui_color(axv::UiRole::DisabledText)
        : (selected_tab || id == kCallButton
           ? ui_color(axv::UiRole::SelectedText) : ui_color(axv::UiRole::ButtonText));
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, foreground);
    wchar_t title[96] = {};
    GetWindowTextW(item.hwndItem, title, 95);

    if (id >= kDigitStart && id < kDigitStart + 12) {
        const auto index = static_cast<size_t>(id - kDigitStart);
        const auto digit = axv::dialpad_keys()[index];
        const wchar_t primary[2] = {digit, 0};
        RECT number = rect;
        number.bottom = number.top + axv::dpi_pixels(37,g_dpi);
        RECT subtitle = rect;
        subtitle.top += axv::dpi_pixels(36,g_dpi);
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
        InflateRect(&rect,-axv::dpi_pixels(5,g_dpi),-axv::dpi_pixels(5,g_dpi));
        DrawFocusRect(item.hDC, &rect);
    }
}

void refresh_destination_feedback() {
    if (!g_number || !g_status) return;
    wchar_t value[129] = {};
    GetWindowTextW(g_number, value, 129);
    const auto feedback = axv::evaluate_destination(value);
    g_destination_invalid = feedback.kind == axv::DestinationFeedbackKind::Invalid;
    SetWindowTextW(g_status, feedback.message);
    InvalidateRect(g_number, nullptr, TRUE);
}

void edit_number(HWND root, wchar_t action) {
    if (!g_number) return;
    wchar_t current[129] = {};
    GetWindowTextW(g_number, current, 129);
    DWORD selection_start = 0, selection_end = 0;
    SendMessageW(g_number, EM_GETSEL, reinterpret_cast<WPARAM>(&selection_start),
                 reinterpret_cast<LPARAM>(&selection_end));
    const auto plan = axv::plan_dial_edit(current, selection_start, selection_end, action);
    if (!plan.accepted) return;
    SetFocus(g_number);
    SendMessageW(g_number, EM_SETSEL, static_cast<WPARAM>(plan.start),
                 static_cast<LPARAM>(plan.end));
    SendMessageW(g_number, EM_REPLACESEL, TRUE,
                 reinterpret_cast<LPARAM>(plan.replacement.c_str()));
    // EM_REPLACESEL triggers EN_CHANGE and preserves native undo.
    (void)root;
}


// Local workspace actions never authorize SIP commands or imply live calls.
std::wstring current_destination() {
    if (!g_number) return {};
    wchar_t value[129] = {};
    GetWindowTextW(g_number, value, 129);
    return value;
}
void recall_destination(const std::wstring& value) {
    if (!g_number || axv::valid_dial_destination(value).empty()) return;
    SetWindowTextW(g_number, value.c_str());
    SetFocus(g_number);
    SendMessageW(g_number, EM_SETSEL, static_cast<WPARAM>(value.size()),
                 static_cast<LPARAM>(value.size()));
}
void remember_current_destination() {
    if (!g_private_dial) {
        axv::remember_dial_destination(g_recent_destinations, current_destination());
    }
}
void show_recent_destinations(HWND window) {
    if (g_private_dial) {
        SetWindowTextW(g_status, L"Modo privado ativo: recentes ocultos e desativados.");
        return;
    }
    remember_current_destination();
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    if (g_recent_destinations.empty()) {
        AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, L"Nenhum destino nesta sessao");
    } else {
        for (size_t i = 0; i < g_recent_destinations.size(); ++i)
            AppendMenuW(menu, MF_STRING, kRecentPickStart + static_cast<UINT>(i),
                        g_recent_destinations[i].c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kClearRecent, L"Limpar destinos recentes");
    }
    const HWND anchor = GetDlgItem(window, kMoreButton);
    RECT pos = {};
    if (anchor) GetWindowRect(anchor, &pos);
    else GetWindowRect(window, &pos);
    SetForegroundWindow(window);
    const UINT choice = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                       pos.left, pos.bottom, 0, window, nullptr);
    DestroyMenu(menu);
    if (choice) SendMessageW(window, WM_COMMAND, MAKEWPARAM(choice, BN_CLICKED), 0);
}

// Copies a redacted LOCAL status only; never read the account, dial field,
// Windows identity, endpoint names, SIP URI or credentials here.
std::wstring safe_desktop_diagnostics() {
    const auto process = g_baresip_process.poll();
    const wchar_t* process_text =
        process == axv::ProcessStatus::Running ? L"running-unverified" :
        process == axv::ProcessStatus::Stopped ? L"stopped" :
        process == axv::ProcessStatus::Failed ? L"failed" :
        process == axv::ProcessStatus::Exited ? L"exited" : L"stopping";
    return std::wstring(L"AXE Voice Desktop LAB 0.139\r\n") +
        L"Processo Baresip: " + process_text +
        L"\r\nRegistro SIP confirmado: NAO\r\nChamada RTP/SRTP homologada: NAO\r\n" +
        L"Autostart: " + (axv::desktop_autostart_enabled() ? L"sim" : L"nao") +
        L"\r\nInicializacao minimizada: " +
        (axv::desktop_autostart_starts_in_tray() ? L"sim" : L"nao") +
        L"\r\nFechar para bandeja: " + (g_close_to_tray ? L"sim" : L"nao") +
        L"\r\nPrivacidade visual: " + (g_private_dial ? L"sim" : L"nao") +
        L"\r\nModo compacto: " + (g_compact ? L"sim" : L"nao") +
        L"\r\nEntradas de audio enumeradas: " + std::to_wstring(g_audio_inputs.size()) +
        L"\r\nSaidas de audio enumeradas: " + std::to_wstring(g_audio_outputs.size()) +
        L"\r\nObs.: inventario local nao confirma audio em chamada.\r\n";
}

bool copy_dial_destination(HWND owner) {
    if (g_smoke_mode || g_private_dial) return false;
    const auto text = axv::valid_dial_destination(current_destination());
    if (text.empty()) return false;
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!buffer) return false;
    wchar_t* data = static_cast<wchar_t*>(GlobalLock(buffer));
    if (!data) { GlobalFree(buffer); return false; }
    std::copy(text.c_str(), text.c_str() + text.size() + 1, data);
    GlobalUnlock(buffer);
    if (!OpenClipboard(owner)) { GlobalFree(buffer); return false; }
    const bool cleared = EmptyClipboard() != FALSE;
    const bool copied = cleared && SetClipboardData(CF_UNICODETEXT, buffer) != nullptr;
    CloseClipboard();
    if (!copied) GlobalFree(buffer);
    return copied;
}

bool paste_dial_destination(HWND owner) {
    if (g_smoke_mode || g_private_dial || !g_number) return false;
    if (!OpenClipboard(owner)) return false;
    std::wstring value;
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h) {
        const SIZE_T capacity = GlobalSize(h) / sizeof(wchar_t);
        if (capacity > 0 && capacity <= 129) {
            const auto* data = static_cast<const wchar_t*>(GlobalLock(h));
            if (data) {
                const auto* end = std::wmemchr(data, L'\0', capacity);
                if (end && end != data) value.assign(data, end);
                GlobalUnlock(h);
            }
        }
    }
    CloseClipboard();
    value = axv::valid_dial_destination(value);
    if (value.empty()) return false;
    recall_destination(value); // Never starts a SIP call.
    return true;
}

bool copy_safe_diagnostics(HWND owner) {
    const std::wstring report = safe_desktop_diagnostics();
    // PR #21: restrict even redacted text before transferring clipboard
    // ownership; do not allow arbitrary large blobs or empty reports.
    if (report.empty() || report.size() > 8192) return false;
    const SIZE_T bytes = (report.size() + 1) * sizeof(wchar_t);
    HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!buffer) return false;
    auto* chars = static_cast<wchar_t*>(GlobalLock(buffer));
    if (!chars) { GlobalFree(buffer); return false; }
    std::copy(report.begin(), report.end(), chars);
    chars[report.size()] = L'\0';
    GlobalUnlock(buffer);
    if (!OpenClipboard(owner)) { GlobalFree(buffer); return false; }
    if (!EmptyClipboard()) { CloseClipboard(); GlobalFree(buffer); return false; }
    const bool transferred = SetClipboardData(CF_UNICODETEXT, buffer) != nullptr;
    CloseClipboard();
    if (!transferred) GlobalFree(buffer);
    return transferred;
}

void show_more_actions(HWND window) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuW(menu, MF_STRING | (g_private_dial ? MF_GRAYED : 0),
                kSaveFavorite, L"Salvar destino como favorito (Ctrl+D)");
    AppendMenuW(menu, MF_STRING | ((g_favorite_destination.empty() || g_private_dial) ? MF_GRAYED : 0),
                kLoadFavorite, L"Carregar favorito (Ctrl+F)");
    AppendMenuW(menu, MF_STRING | ((g_favorite_destination.empty() || g_private_dial) ? MF_GRAYED : 0),
                kDeleteFavorite, L"Remover favorito salvo");
    AppendMenuW(menu, MF_STRING | (g_private_dial ? MF_GRAYED : 0),
                kShowRecent, L"Destinos recentes (Ctrl+H)");
    AppendMenuW(menu, MF_STRING | ((g_private_dial || g_recent_destinations.empty()) ? MF_GRAYED : 0),
                kRecallLastDestination, L"Recuperar ultimo destino (Ctrl+Shift+R)");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (g_private_dial ? MF_GRAYED : 0),
                kCopyDestination, L"Copiar destino validado");
    AppendMenuW(menu, MF_STRING | (g_private_dial ? MF_GRAYED : 0),
                kPasteDestination, L"Colar destino validado");
    AppendMenuW(menu, MF_STRING, kSelectAllDestination, L"Selecionar todo destino");
    AppendMenuW(menu, MF_STRING, kUndoDialEdit, L"Desfazer edicao do destino");
    AppendMenuW(menu, MF_STRING, kResetDialWorkspace, L"Limpar destino e recentes...");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (g_private_dial ? MF_CHECKED : 0),
                kPrivacyDial, L"Ocultar destino na tela (Ctrl+Shift+P)");
    AppendMenuW(menu, MF_STRING | (g_close_to_tray ? MF_CHECKED : 0),
                kCloseToTrayToggle, L"Fechar janela para bandeja");
    AppendMenuW(menu, MF_STRING | (axv::desktop_autostart_enabled() ? MF_CHECKED : 0),
                kAutostartToggle, L"Iniciar com Windows (usuario atual)");
    AppendMenuW(menu, MF_STRING | (!axv::desktop_autostart_enabled() ? MF_GRAYED : 0) |
                      (axv::desktop_autostart_starts_in_tray() ? MF_CHECKED : 0),
                kStartupInTrayToggle, L"Iniciar minimizado na bandeja");
    AppendMenuW(menu, MF_STRING, kCopySafeDiagnostics, L"Copiar diagnostico local sem dados pessoais");
    AppendMenuW(menu, MF_STRING, kSipPreflight, L"Pre-ativacao SIP segura (F3)");
    AppendMenuW(menu, MF_STRING, kSipVaultStatus, L"Verificar credencial SIP no Windows (F4)");
    AppendMenuW(menu, MF_STRING, kOpenCredentialManager, L"Abrir Gerenciador de Credenciais (F5)");
    AppendMenuW(menu, MF_STRING, kRemoveSipVaultCredential, L"Remover credencial SIP do Windows (F7)");
    AppendMenuW(menu, MF_STRING, kSipNextSteps, L"Pendencias para chamadas SIP (F6)");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kExplicitExit, L"Sair do AXE Voice");
    const HWND anchor = GetDlgItem(window, kMoreButton);
    RECT pos = {};
    if (anchor) GetWindowRect(anchor, &pos);
    else GetWindowRect(window, &pos);
    SetForegroundWindow(window);
    const UINT choice = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                       pos.left, pos.bottom, 0, window, nullptr);
    DestroyMenu(menu);
    if (choice) SendMessageW(window, WM_COMMAND, MAKEWPARAM(choice, BN_CLICKED), 0);
}

// Audio preferences are previews only: they never configure or contact Baresip.
axv::AudioSelectionRefresh fill_audio_combo(
    HWND combo, const std::vector<axv::AudioEndpoint>& devices,
    std::wstring& selected_id, bool ringtone = false) {
    const auto selection = ringtone
        ? axv::refresh_ringtone_selection(devices, selected_id)
        : axv::refresh_audio_selection(devices, selected_id);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    for (const auto& device : devices) {
        const std::wstring prefix = ringtone
            ? (device.default_multimedia ? L"[PADRAO TOQUE] " : L"")
            : (device.default_communications ? L"[PADRAO] " : L"");
        const std::wstring title = prefix + device.name;
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(title.c_str()));
    }
    EnableWindow(combo, selection.index >= 0);
    if (selection.index >= 0) {
        SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(selection.index), 0);
    }
    selected_id = selection.selected_id;
    return selection;
}


// UI-only: there is no Baresip routing or actual ringtone in this build.
void update_swap_button() {
    if (!g_output_audio_combo) return;
    HWND parent = GetParent(g_output_audio_combo);
    if (!parent) return;
    const auto plan = axv::plan_audio_role_swap(
        g_audio_outputs, g_selected_output_id, g_selected_ringtone_id);
    EnableWindow(GetDlgItem(parent, kSwapAudioRolesButton),
                 plan.can_swap && !g_preview_running.load() && !g_preview_pending);
}

void update_audio_preview_buttons(HWND window) {
    const bool running = g_preview_running.load();
    const bool idle = !running && !g_preview_pending && !g_smoke_mode;
    const bool has_voice = axv::select_audio_preview_target(
        g_audio_outputs, g_selected_output_id, g_selected_ringtone_id,
        axv::AudioPreviewRole::Voice).available;
    const bool has_ring = axv::select_audio_preview_target(
        g_audio_outputs, g_selected_output_id, g_selected_ringtone_id,
        axv::AudioPreviewRole::Ringtone).available;
    HWND voice = GetDlgItem(window, kTestVoiceButton);
    HWND ring = GetDlgItem(window, kTestRingtoneButton);
    const bool stop_voice = axv::preview_can_cancel(running,
        g_preview_cancel.load(), g_smoke_mode, g_preview_role,
        axv::AudioPreviewRole::Voice);
    const bool stop_ring = axv::preview_can_cancel(running,
        g_preview_cancel.load(), g_smoke_mode, g_preview_role,
        axv::AudioPreviewRole::Ringtone);
    if (voice) {
        SetWindowTextW(voice, running && g_preview_role == axv::AudioPreviewRole::Voice
            ? (g_preview_cancel.load() ? L"PARANDO VOZ..." : L"PARAR VOZ")
            : L"TESTAR VOZ");
        EnableWindow(voice, stop_voice || (idle && has_voice));
    }
    if (ring) {
        SetWindowTextW(ring, running && g_preview_role == axv::AudioPreviewRole::Ringtone
            ? (g_preview_cancel.load() ? L"PARANDO TOQUE..." : L"PARAR TOQUE")
            : L"TESTAR TOQUE");
        EnableWindow(ring, stop_ring || (idle && has_ring));
    }
}

void refresh_audio_panel() {
    // An explicit refresh or tab visit supersedes any pending hotplug hint.
    g_audio_hotplug.acknowledge_manual_refresh();
    if (g_preview_running.load()) g_preview_cancel.store(true);
    // A new inventory can invalidate the prior incoming-ringtone route.
    (void)g_incoming_ring_coordinator.revoke(g_incoming_ring_sink);
    const auto inventory = axv::inspect_windows_audio_devices();
    if (inventory.error != axv::AudioInventoryError::None) {
        // Port of PR #18: a failed Core Audio query says nothing about device
        // removal. Disable previews and selectors, but keep both active and
        // explicitly preferred IDs in RAM for the next successful query.
        g_audio_inputs.clear(); g_audio_outputs.clear();
        for (HWND combo : {g_input_audio_combo, g_output_audio_combo,
                           g_ringtone_audio_combo}) {
            if (combo) {
                SendMessageW(combo, CB_RESETCONTENT, 0, 0);
                EnableWindow(combo, FALSE);
            }
        }
        update_swap_button();
        update_audio_preview_buttons(GetParent(g_ringtone_audio_combo));
        SetWindowTextW(g_headset_status, L"Falha ao consultar os dispositivos do Windows.");
        return;
    }
    g_audio_inputs = inventory.input_devices;
    g_audio_outputs = inventory.output_devices;
    // Port of PR #16: prefer an explicitly selected endpoint whenever it
    // returns. Temporary fallback remains active only until reconnection.
    if (!g_preferred_input_id.empty())
        g_selected_input_id = g_preferred_input_id;
    if (!g_preferred_output_id.empty())
        g_selected_output_id = g_preferred_output_id;
    if (!g_preferred_ringtone_id.empty())
        g_selected_ringtone_id = g_preferred_ringtone_id;
    const auto microphone = fill_audio_combo(
        g_input_audio_combo, g_audio_inputs, g_selected_input_id);
    const auto speaker = fill_audio_combo(
        g_output_audio_combo, g_audio_outputs, g_selected_output_id);
    const auto ringtone = fill_audio_combo(
        g_ringtone_audio_combo, g_audio_outputs, g_selected_ringtone_id, true);
    update_swap_button();
    update_audio_preview_buttons(GetParent(g_ringtone_audio_combo));
    SetWindowTextW(g_headset_status,
        axv::audio_routing_preview_feedback({microphone, speaker, ringtone}));
}

bool defer_route_while_preview_active(HWND window, const axv::AudioRouteChange& change) {
    if (!g_audio_route_fence.defer_if_busy(change,
            g_preview_running.load() || g_preview_pending))
        return false;
    // The old endpoint must be fully stopped (worker joined) before changing
    // any visible route, including the two independently selected outputs.
    g_preview_cancel.store(true);
    SetWindowTextW(g_headset_status,
        L"Parando teste local antes de mudar dispositivo. Aguarde confirmacao.");
    update_swap_button();
    update_audio_preview_buttons(window);
    return true;
}

void apply_audio_route_change(HWND window, const axv::AudioRouteChange& change) {
    using Kind = axv::AudioRouteChangeKind;
    if (change.kind == Kind::None) return;
    if (change.kind == Kind::Refresh) {
        refresh_audio_panel();
        return;
    }
    if (change.kind == Kind::Swap) {
        const auto plan = axv::plan_audio_role_swap(
            g_audio_outputs, g_selected_output_id, g_selected_ringtone_id);
        if (!plan.can_swap) {
            update_swap_button();
            SetWindowTextW(g_headset_status,
                L"Troca indisponivel: escolha duas saidas ativas e distintas. Sem audio SIP.");
            return;
        }
        (void)g_incoming_ring_coordinator.revoke(g_incoming_ring_sink);
        g_selected_output_id = plan.next_call_id;
        g_selected_ringtone_id = plan.next_ringtone_id;
        g_preferred_output_id = g_selected_output_id;
        g_preferred_ringtone_id = g_selected_ringtone_id;
        SendMessageW(g_output_audio_combo, CB_SETCURSEL,
                     static_cast<WPARAM>(plan.next_call_index), 0);
        SendMessageW(g_ringtone_audio_combo, CB_SETCURSEL,
                     static_cast<WPARAM>(plan.next_ringtone_index), 0);
        update_swap_button();
        update_audio_preview_buttons(window);
        SetWindowTextW(g_headset_status,
            L"Toque e voz invertidos apos parada confirmada. Sem audio SIP.");
        return;
    }
    HWND combo = change.kind == Kind::Microphone ? g_input_audio_combo :
                 change.kind == Kind::Voice ? g_output_audio_combo : g_ringtone_audio_combo;
    const auto& devices = change.kind == Kind::Microphone ? g_audio_inputs : g_audio_outputs;
    int index = -1;
    if (!change.endpoint_id.empty() && change.endpoint_id.size() <= 4096) {
        for (std::size_t i = 0; i < devices.size(); ++i)
            if (devices[i].id == change.endpoint_id) {
                index = static_cast<int>(i);
                break;
            }
    }
    if (index < 0) {
        // Never route to a stale index after a hotplug or inventory refresh.
        refresh_audio_panel();
        SetWindowTextW(g_headset_status,
            L"Dispositivo nao disponivel. Lista atualizada; selecione novamente.");
        return;
    }
    if (change.kind != Kind::Microphone)
        (void)g_incoming_ring_coordinator.revoke(g_incoming_ring_sink);
    std::wstring& selected = change.kind == Kind::Microphone ? g_selected_input_id :
                             change.kind == Kind::Voice ? g_selected_output_id :
                             g_selected_ringtone_id;
    selected = change.endpoint_id;
    std::wstring& preferred = change.kind == Kind::Microphone ? g_preferred_input_id :
                              change.kind == Kind::Voice ? g_preferred_output_id :
                              g_preferred_ringtone_id;
    preferred = selected; // Only user actions update the durable-in-memory choice.
    SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    const auto routing = axv::preview_audio_routing(
        g_audio_inputs, g_audio_outputs,
        g_selected_input_id, g_selected_output_id, g_selected_ringtone_id);
    SetWindowTextW(g_headset_status, axv::audio_routing_preview_feedback(routing));
    update_swap_button();
    update_audio_preview_buttons(window);
}

void restore_audio_combo_selection(HWND combo,
    const std::vector<axv::AudioEndpoint>& devices, const std::wstring& selected_id) {
    for (std::size_t i = 0; i < devices.size(); ++i)
        if (devices[i].id == selected_id) {
            SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
            return;
        }
    SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
}

axv::RuntimeProcess observed_process(axv::ProcessStatus status, bool stopping) {
    if (stopping) return axv::RuntimeProcess::Stopping;
    switch (status) {
    case axv::ProcessStatus::Running: return axv::RuntimeProcess::Running;
    case axv::ProcessStatus::Failed: return axv::RuntimeProcess::Failed;
    case axv::ProcessStatus::Stopped:
    case axv::ProcessStatus::Exited: return axv::RuntimeProcess::Stopped;
    }
    return axv::RuntimeProcess::Failed;
}
void refresh_runtime_banner(axv::ProcessStatus state) {
    if (!g_runtime_banner) return;
    // Thread-safe receive boundary is active, but not connected to Baresip C API.
    // Bind native event lifetime to actual process lifetime, not to a timer tick.
    const auto& observed = g_sip_session_guard.observe(
        observed_process(state, g_shutdown.pending()), axv::SipEvidence::None,
        g_native_event_mailbox, g_sip_projection);
    const auto current_session = observed.session;
    const auto ring_target = axv::select_audio_preview_target(
        g_audio_outputs, g_selected_output_id, g_selected_ringtone_id,
        axv::AudioPreviewRole::Ringtone);
    // A prospective incoming ringtone is authorized only from a verified
    // in-process SIP event and the *current* Windows output endpoint.
    // LAB supplies SipEvidence::None; no Start can produce sound.
    const auto ring_action = g_incoming_ring_coordinator.update(
        observed, current_session, g_selected_ringtone_id,
        ring_target.available, g_incoming_ring_sink);
    (void)ring_action; // Disabled sink; no automatic SIP ringtone in LAB.
    // STOP must be acknowledged before a different device can START.
    // No audited native producer is linked; process state never implies SIP proof.
    // Without a native verified producer the UI cannot claim SIP registration.
    const wchar_t* banner = axv::runtime_status_banner(observed, current_session);
    wchar_t previous[160]{};
    GetWindowTextW(g_runtime_banner, previous, 160);
    if (std::wstring_view(previous) != std::wstring_view(banner))
        SetWindowTextW(g_runtime_banner, banner);
    wchar_t destination[129]{};
    GetWindowTextW(g_number, destination, 129);
    // The click handler uses the same gate, with a fresh process observation.
    // Current UnavailableSipEngine is always denied, even for valid destinations.
    const bool dial = axv::authorize_dial(
        observed, current_session, g_sip_selection, destination).allowed();
    const HWND button = GetDlgItem(GetParent(g_runtime_banner), kCallButton);
    if (IsWindowEnabled(button) != (dial ? TRUE : FALSE))
        EnableWindow(button, dial ? TRUE : FALSE);
}

// Inspect local prerequisite SHAPES only. No SIP SDK, network, credentials,
// native owner callbacks or real call proof are available in this GUI build.

// User-invoked local readiness action. Identity is displayed ONLY in this
// dedicated dialog (never F3, logs, clipboard diagnostics or phone history).
// Reading Credential Manager only checks presence; it cannot prove auth.
std::wstring make_sip_vault_report() {
    wchar_t server[129] = {}, user[129] = {};
    GetWindowTextW(g_server, server, 129);
    GetWindowTextW(g_user, user, 129);
    const auto status = axv::inspect_sip_vault_presence(server, user);
    std::wstring report = L"AXE Voice LAB - Cofre do Windows\r\n\r\n";
    report += axv::sip_vault_status_label(status);
    report += L"\r\n\r\n";
    if (status == axv::SipVaultStatus::PresentUnverified ||
        status == axv::SipVaultStatus::Missing) {
        const auto alias = axv::sip_vault_alias(user, server);
        if (!alias.empty()) {
            report += L"Nome para criar uma Credencial Generica no Windows:\r\n";
            report += alias;
            report += L"\r\n\r\n";
        }
    }
    report += L"Use o Gerenciador de Credenciais do Windows para criar\r\n";
    report += L"uma credencial generica com o nome acima.\r\n";
    report += L"Nenhuma senha e exibida ou copiada pelo AXE Voice.\r\n";
    report += L"A presenca de um segredo NAO comprova LOGIN SIP.\r\n";
    report += L"O SDK SIP ainda nao consome estas credenciais.\r\n";
    return report;
}


// Clear operator guidance derived only from local state. Never reports
// registration success merely because credentials/files/devices exist.
std::wstring make_sip_next_steps_report() {
    wchar_t server[129] = {}, user[129] = {}, port[129] = {};
    if (g_server) GetWindowTextW(g_server, server, 129);
    if (g_user) GetWindowTextW(g_user, user, 129);
    if (g_port) GetWindowTextW(g_port, port, 129);
    const bool selected_tls = g_tls &&
        SendMessageW(g_tls, BM_GETCHECK, 0, 0) == BST_CHECKED;
    const auto local = axv::evaluate_native_sip_preflight(
        server, user, port, selected_tls);
    const bool identity_ready = local.eligible_for_preparation;
    const auto vault = identity_ready
        ? axv::inspect_sip_vault_presence(server, user)
        : axv::SipVaultStatus::AccountInvalid;
    const bool vault_ready = vault == axv::SipVaultStatus::PresentUnverified;
    std::wstring out = L"AXE VOICE 0.103 - ETAPAS PARA USO SIP\r\n\r\n";
    out += L"1. Conta SIP/TLS: ";
    out += identity_ready ? L"formato local aceito (NAO testado)\r\n" :
                           L"revisar campos/selecionar TLS (F3)\r\n";
    out += L"2. Cofre Windows: ";
    out += !identity_ready ? L"depende de identidade valida\r\n" :
           vault_ready ? L"credencial presente, sem prova de senha\r\n" :
                         L"revisar credencial generica (F4/F5)\r\n";
    out += L"3. SDK Baresip/re auditado: PENDENTE NO EXECUTAVEL\r\n";
    out += L"4. Autenticacao real/TLS do PABX: NAO VERIFICADA\r\n";
    out += L"5. RTP/SRTP, codecs e audio headset: NAO VERIFICADOS\r\n";
    out += L"6. Build e homologacao Windows/PABX: PENDENTES\r\n\r\n";
    out += L"CHAMADAS BLOQUEADAS: motor SIP nativo nao ligado a GUI.\r\n";
    out += L"Este relatorio nao divulga dominio, ramal ou senha.\r\n";
    return out;
}

std::wstring make_native_sip_preflight_report() {
    wchar_t server[129] = {}, user[129] = {}, port[129] = {};
    if (g_server) GetWindowTextW(g_server, server, 129);
    if (g_user) GetWindowTextW(g_user, user, 129);
    if (g_port) GetWindowTextW(g_port, port, 129);
    const bool tls_checked = g_tls &&
        SendMessageW(g_tls, BM_GETCHECK, 0, 0) == BST_CHECKED;
    const auto account = axv::evaluate_native_sip_preflight(
        server, user, port, tls_checked);
    const axv::NativeSipReleaseEvidence evidence{}; // no approvals available in GUI
    const auto commercial_gate = axv::evaluate_native_sip_activation(evidence);
    (void)commercial_gate;

    wchar_t exe[MAX_PATH] = {};
    const DWORD len = GetModuleFileNameW(nullptr, exe, MAX_PATH);
    bool has_local_baresip = false;
    axv::BaresipModuleInventory inventory{};
    if (len > 0 && len < MAX_PATH) {
        const auto candidate = std::filesystem::path(exe).parent_path() / L"baresip.exe";
        const auto probe = axv::probe_baresip_executable(candidate);
        has_local_baresip =
            probe.status == axv::BaresipProbeStatus::PresentUnverified;
        if (has_local_baresip)
            inventory = axv::inspect_baresip_module_files(candidate);
    }

    std::wstring report =
        L"AXE VOICE LAB - PRE-ATIVACAO SIP (v0.103)\r\n\r\n";
    report += L"1. Dados de conta: ";
    report += axv::native_preflight_label(account.status);
    report += L"\r\n2. Transporte SIP TLS: ";
    report += tls_checked ? L"selecionado (nao testado)" : L"nao selecionado";
    report += L"\r\n3. Baresip local: ";
    report += has_local_baresip ? L"arquivo encontrado (nao autenticado)" :
                                  L"nao localizado/verificavel";
    report += L"\r\n4. Modulos locais: ";
    report += inventory.status == axv::BaresipModuleScanStatus::Inspected
        ? L"arquivos inspecionados (sem prova de codecs)" : L"inspecao incompleta";
    report += L"\r\n5. Audio Windows: ";
    report += !g_audio_inputs.empty() && !g_audio_outputs.empty()
        ? L"dispositivos enumerados (sem prova de RTP)" : L"sem inventario completo";
    report += L"\r\n\r\nEVIDENCIAS OPERACIONAIS\r\n";
    report += L"Motor SDK conectado ao aplicativo: NAO\r\n";
    report += L"Autenticacao com PABX validada: NAO\r\n";
    report += L"TLS do servidor certificado: NAO\r\n";
    report += L"Midia SRTP bidirecional validada: NAO\r\n";
    report += L"Release comercial autorizada: NAO\r\n";
    report += L"Comandos de chamada: BLOQUEADOS\r\n\r\n";
    report += L"Nenhuma credencial, dominio, ramal ou URI e exibida.\r\n";
    return report;
}

std::wstring make_readiness_report() {
    wchar_t number[129]{}, server[129]{}, user[129]{}, port[129]{};
    GetWindowTextW(g_number, number, 129);
    GetWindowTextW(g_server, server, 129);
    GetWindowTextW(g_user, user, 129);
    GetWindowTextW(g_port, port, 129);
    const auto& snapshot = g_sip_projection.snapshot();
    axv::CallReadiness result{};
    result.destination_syntax = axv::evaluate_destination(number).valid();
    result.account_syntax = axv::evaluate_account_form(server, user, port).valid();
    result.native_engine = g_sip_selection.kind == axv::EngineKind::Baresip &&
        g_sip_selection.engine &&
        g_sip_selection.engine->status() != axv::SipStatus::Unavailable;
    result.running_process = snapshot.process == axv::RuntimeProcess::Running;
    result.verified_registration = result.native_engine &&
        axv::sip_proof_is_current(snapshot, snapshot.session) &&
        snapshot.sip == axv::SipStatus::Registered;
    // A device inventory is not an RTP/SRTP or headset media test.
    result.verified_media = false;
    return axv::format_readiness_export(result);
}
void begin_shutdown(HWND window) {
    g_sip_session_guard.revoke(axv::RuntimeProcess::Stopping, g_native_event_mailbox, g_sip_projection);
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
        g_dpi=axv::normalized_dpi(GetDpiForWindow(window));
        load_ui_preferences();
        read_windows_contrast();
        g_font_regular = create_font(17, FW_MEDIUM);
        g_font_title = create_font(27, FW_BOLD);
        g_font_small = create_font(13, FW_NORMAL);
        g_font_digit = create_font(25, FW_SEMIBOLD);
        refresh_brushes();
        make_label(window, L"AXE  VOICE", 24, 16, 240, 37, -1, g_font_title);
        make_label(window, L"SOFTPHONE DESKTOP  /  LAB", 25, 57, 270, 22, -1, g_font_small);
        make_button(window, g_compact ? L"MAX" : L"MINI", kCompact, 344, 65, 70, 29);
        make_button(window, L"?", kHelpButton, 301, 65, 34, 29);
        make_button(window, g_pinned ? L"SOLTAR" : L"FIXAR", kPin, 265, 18, 70, 36);
        make_button(window, g_dark ? L"CLARO" : L"ESCURO", kTheme, 344, 18, 70, 36);
        if (g_pinned) SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
                                   SWP_NOMOVE | SWP_NOSIZE);
        make_button(window, L"TECLADO", kShowDial, 24, 103, 122, 42);
        make_button(window, L"AJUSTES", kShowSettings, 153, 103, 123, 42);
        make_button(window, L"AUDIO", kShowAudio, 283, 103, 131, 42);
        g_runtime_banner = make_label(window, L"MOTOR PARADO  |  SIP NAO REGISTRADO",
                                     42, 164, 347, 25, -1, g_font_regular);

        make_label(window, L"DISCAGEM", 39, 220, 290, 23, 0, g_font_small);
        g_number_label = make_label(window, L"Número ou endereço SIP", 39, 246, 320, 20, 0, g_font_regular);
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
        make_button(window, L"APAGAR", kBackspace, 39, 597, 110, 38, 0);
        make_button(window, L"LIMPAR", kClear, 155, 597, 110, 38, 0);
        make_button(window, L"MAIS", kMoreButton, 272, 597, 117, 38, 0);
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
        make_label(window, L"HEADSET / AUDIO - TRES ROTAS", 39, 218, 352, 27, 2, g_font_small);
        make_label(window, L"Microfone (captura)", 39, 256, 340, 24, 2);
        g_input_audio_combo = make_control(window, L"COMBOBOX", L"",
            WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            39, 282, 350, 200, kInputAudioCombo, 2);
        make_label(window, L"Voz da conversa (fone / alto-falante)", 39, 332, 350, 25, 2);
        g_output_audio_combo = make_control(window, L"COMBOBOX", L"",
            WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            39, 359, 350, 200, kOutputAudioCombo, 2);
        make_label(window, L"Toque de chamada (saída independente)", 39, 407, 350, 26, 2);
        g_ringtone_audio_combo = make_control(window, L"COMBOBOX", L"",
            WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            39, 435, 350, 200, kRingtoneAudioCombo, 2);
        make_button(window, L"ATUALIZAR", kRefreshHeadsetsButton,
                    39, 507, 170, 43, 2);
        make_button(window, L"INVERTER VOZ/TOQUE", kSwapAudioRolesButton,
                    218, 507, 171, 43, 2);
        make_button(window, L"TESTAR VOZ", kTestVoiceButton,
                    39, 559, 170, 43, 2);
        make_button(window, L"TESTAR TOQUE", kTestRingtoneButton,
                    218, 559, 171, 43, 2);
        make_label(window, L"VOLUME TESTE - VOZ", 39, 606, 170, 20, 2, g_font_small);
        make_label(window, L"VOLUME TESTE - TOQUE", 218, 606, 171, 20, 2, g_font_small);
        g_voice_level_combo = make_control(window, L"COMBOBOX", L"",
            WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            39, 630, 170, 145, kVoicePreviewLevel, 2);
        g_ring_level_combo = make_control(window, L"COMBOBOX", L"",
            WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            218, 630, 171, 145, kRingPreviewLevel, 2);
        for (HWND combo : {g_voice_level_combo, g_ring_level_combo}) {
            for (const wchar_t* item : {L"25%", L"50%", L"75%", L"100%"})
                SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
            SendMessageW(combo, CB_SETCURSEL,
                static_cast<WPARAM>(axv::preview_level_index(axv::kPreviewDefaultLevel)), 0);
        }
        g_headset_status = make_label(window, L"Volumes do teste local. Sem chamada SIP.",
                                     39, 669, 350, 38, 2, g_font_small);
        make_button(window, L"PRONTIDAO DE CHAMADAS (F2)",
                    kReadinessButton, 39, 710, 350, 38, 2);
        g_status = make_label(window, L"Digite um ramal ou endereço SIP.",
                              24, 762, 390, 30, -1, g_font_small);
        refresh_destination_feedback();

        EnableWindow(g_input_audio_combo, FALSE);
        EnableWindow(g_output_audio_combo, FALSE);
        EnableWindow(g_ringtone_audio_combo, FALSE);
        update_swap_button();
        update_audio_preview_buttons(window);
        // A clear disabled affordance: no real SIP engine has been connected yet.
        for (int id : {kCallButton, kMuteFuture, kHoldFuture,
                       kTransferFuture, kHangupFuture}) {
            EnableWindow(GetDlgItem(window, id), FALSE);
        }
        show_page(window, 0);
        apply_compact_layout(window);
        refresh_runtime_banner(g_baresip_process.poll());
        SetTimer(window, kProcessTimer, 100, nullptr);
        g_controls_ready = true;
        sync_viewport_scroll(window);
        return 0;
    }
    case WM_DEVICECHANGE:
        // Generic Win32 device changes are hints, NOT proof of audio hotplug.
        // Do not enumerate here: the UI timer coalesces notifications.
        if (g_audio_hotplug.notify(static_cast<unsigned>(wparam), GetTickCount64())) {
            if (g_page == 2 && !g_smoke_mode && g_headset_status)
                SetWindowTextW(g_headset_status,
                    L"Possivel mudanca de dispositivos; atualizacao pendente...");
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (lparam && (HIWORD(wparam)==EN_SETFOCUS ||
                       HIWORD(wparam)==BN_SETFOCUS ||
                       HIWORD(wparam)==CBN_SETFOCUS))
            bring_focused_control_into_view(window,reinterpret_cast<HWND>(lparam));
        if (LOWORD(wparam) == kNumberEdit && HIWORD(wparam) == EN_KILLFOCUS) {
            remember_current_destination();
            return 0;
        }
        if (LOWORD(wparam) == kNumberEdit && HIWORD(wparam) == EN_CHANGE) {
            refresh_destination_feedback(); // keyboard, paste and dialpad share validation.
            return 0;
        }
        if (HIWORD(wparam) == EN_CHANGE &&
            (LOWORD(wparam) == kServerEdit || LOWORD(wparam) == kUserEdit ||
             LOWORD(wparam) == kPortEdit)) {
            if (g_page == 1) refresh_account_feedback();
            return 0;
        }
        // Combo selection uses code 1 too; accelerator commands have lParam == 0.
        if (HIWORD(wparam) == BN_CLICKED || (HIWORD(wparam) == 1 && lparam == 0)) {
            const int control = LOWORD(wparam);
            if (control == kShowDial || control == kFocusNumber) { show_page(window, 0); return 0; }
            if (control == kMoreButton) { show_more_actions(window); return 0; }
            if (control == kShowRecent) { show_recent_destinations(window); return 0; }
            if (control == kClearRecent) {
                g_recent_destinations.clear();
                SetWindowTextW(g_status, L"Destinos recentes desta sessao apagados.");
                return 0;
            }
            if (!g_private_dial && control >= kRecentPickStart && control < kRecentPickStart + 10) {
                const size_t index = static_cast<size_t>(control - kRecentPickStart);
                if (index < g_recent_destinations.size()) {
                    show_page(window, 0);
                    recall_destination(g_recent_destinations[index]);
                }
                return 0;
            }
            if (control == kCopyDestination) {
                if (g_private_dial || g_smoke_mode) return 0;
                SetWindowTextW(g_status, copy_dial_destination(window)
                    ? L"Destino copiado. A area de transferencia e compartilhada com outros apps."
                    : L"Falha: informe destino valido e verifique a area de transferencia.");
                return 0;
            }
            if (control == kPasteDestination) {
                if (g_private_dial || g_smoke_mode) return 0;
                const bool ok = paste_dial_destination(window);
                SetWindowTextW(g_status, ok
                    ? L"Destino validado e colado. Nenhuma chamada foi iniciada."
                    : L"Clipboard vazio, excessivo ou invalido. Nada foi colado.");
                return 0;
            }
            if (control == kSelectAllDestination) {
                show_page(window, 0);
                SendMessageW(g_number, EM_SETSEL, 0, -1);
                return 0;
            }
            if (control == kUndoDialEdit) {
                show_page(window, 0);
                SendMessageW(g_number, EM_UNDO, 0, 0);
                return 0;
            }
            if (control == kResetDialWorkspace) {
                if (!g_smoke_mode && MessageBoxW(window,
                    L"Apagar destino atual e destinos recentes desta sessao?",
                    L"Limpar espaco de discagem", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
                    return 0;
                g_recent_destinations.clear();
                if (g_number) SetWindowTextW(g_number, L"");
                SetWindowTextW(g_status, L"Campo e recentes limpos. Favorito preservado.");
                return 0;
            }
            if (control == kSaveFavorite) {
                if (g_private_dial) {
                    SetWindowTextW(g_status, L"Modo privado: desative-o antes de salvar favorito.");
                    return 0;
                }
                const auto value = axv::valid_dial_destination(current_destination());
                if (value.empty()) {
                    SetWindowTextW(g_status, L"Digite um destino valido antes de salvar favorito.");
                } else if (g_smoke_mode) {
                    g_favorite_destination = value; // no registry changes in smoke mode
                } else if (axv::save_desktop_favorite(kPrefsKey, value)) {
                    g_favorite_destination = value;
                    SetWindowTextW(g_status, L"Favorito salvo neste usuario do Windows.");
                } else {
                    SetWindowTextW(g_status, L"Nao foi possivel salvar favorito local.");
                }
                return 0;
            }
            if (control == kLoadFavorite) {
                if (!g_private_dial && !g_favorite_destination.empty()) {
                    show_page(window, 0);
                    recall_destination(g_favorite_destination);
                }
                return 0;
            }
            if (control == kPrivacyDial) {
                g_private_dial = !g_private_dial;
                if (g_private_dial) g_recent_destinations.clear();
                SendMessageW(g_number, EM_SETPASSWORDCHAR,
                             g_private_dial ? static_cast<WPARAM>(0x2022) : 0, 0);
                InvalidateRect(g_number, nullptr, TRUE);
                SetWindowTextW(g_status, g_private_dial
                    ? L"Modo privado: destino mascarado, recentes limpos e nao registrados."
                    : L"Destino visivel; recentes retomados nesta sessao.");
                return 0;
            }
            if (control == kCloseToTrayToggle) {
                g_close_to_tray = !g_close_to_tray;
                save_ui_preferences();
                SetWindowTextW(g_status, g_close_to_tray
                    ? L"Fechar janela ocultara na bandeja; Sair encerra o programa."
                    : L"Fechar janela encerrara o aplicativo.");
                return 0;
            }
            if (control == kAutostartToggle) {
                if (g_smoke_mode) return 0;
                const bool enable = !axv::desktop_autostart_enabled();
                const bool ok = axv::desktop_set_autostart(enable);
                SetWindowTextW(g_status, !ok ? L"Falha ao alterar inicializacao com Windows."
                    : enable ? L"Inicializacao com Windows habilitada no HKCU."
                             : L"Inicializacao automatica com Windows removida.");
                return 0;
            }
            if (control == kDeleteFavorite) {
                if (g_private_dial) return 0;
                if (g_smoke_mode || axv::desktop_clear_favorite(kPrefsKey)) {
                    g_favorite_destination.clear();
                    SetWindowTextW(g_status, L"Favorito removido deste usuario do Windows.");
                } else {
                    SetWindowTextW(g_status, L"Nao foi possivel remover favorito.");
                }
                return 0;
            }
            if (control == kRecallLastDestination) {
                if (!g_private_dial && !g_recent_destinations.empty()) {
                    show_page(window, 0);
                    recall_destination(g_recent_destinations.front());
                }
                return 0;
            }
            if (control == kCopySafeDiagnostics) {
                if (g_smoke_mode) return 0;
                SetWindowTextW(g_status, copy_safe_diagnostics(window)
                    ? L"Diagnostico local anonimizado copiado para area de transferencia."
                    : L"Falha ao copiar diagnostico para area de transferencia.");
                return 0;
            }
            if (control == kStartupInTrayToggle) {
                if (g_smoke_mode || !axv::desktop_autostart_enabled()) return 0;
                const bool next = !axv::desktop_autostart_starts_in_tray();
                const bool ok = axv::desktop_set_autostart(true, next);
                SetWindowTextW(g_status, !ok
                    ? L"Falha ao alterar a inicializacao minimizada."
                    : next ? L"Windows iniciara AXE Voice diretamente na bandeja."
                           : L"Windows iniciara AXE Voice com janela visivel.");
                return 0;
            }
            if (control == kExplicitExit) {
                g_explicit_exit = true;
                SendMessageW(window, WM_CLOSE, 0, 0);
                return 0;
            }
            if (control == kSipVaultStatus) {
                if (!g_smoke_mode) {
                    const auto report = make_sip_vault_report();
                    MessageBoxW(window, report.c_str(),
                                L"Cofre SIP - AXE Voice LAB", MB_OK | MB_ICONINFORMATION);
                }
                return 0;
            }
            if (control == kRemoveSipVaultCredential) {
                if (g_smoke_mode) return 0;
                const int choice = MessageBoxW(
                    window,
                    L"Excluir a credencial SIP da conta configurada neste usuario"
                    L" do Windows? A acao nao pode ser desfeita pelo AXE Voice."
                    L" Nenhuma senha sera exibida.",
                    L"Confirmar exclusao de credencial SIP",
                    MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                if (choice != IDYES) return 0;
                wchar_t server[129] = {}, user[129] = {};
                GetWindowTextW(g_server, server, 129);
                GetWindowTextW(g_user, user, 129);
                const auto result = axv::remove_sip_vault_credential(server, user);
                SetWindowTextW(g_status, axv::sip_vault_removal_label(result));
                return 0;
            }
            if (control == kOpenCredentialManager) {
                if (g_smoke_mode) return 0;
                const auto status = reinterpret_cast<INT_PTR>(ShellExecuteW(
                    window, L"open", L"control.exe",
                    L"/name Microsoft.CredentialManager", nullptr, SW_SHOWNORMAL));
                if (status <= 32)
                    MessageBoxW(window,
                        L"O Windows nao abriu o Gerenciador de Credenciais. "
                        L"Abra Painel de Controle > Contas de Usuario > "
                        L"Gerenciador de Credenciais.",
                        L"Cofre Windows indisponivel", MB_OK | MB_ICONWARNING);
                return 0;
            }
            if (control == kSipNextSteps) {
                if (!g_smoke_mode) {
                    const auto report = make_sip_next_steps_report();
                    MessageBoxW(window, report.c_str(),
                                L"Pendencias SIP - AXE Voice LAB",
                                MB_OK | MB_ICONINFORMATION);
                }
                return 0;
            }
            if (control == kSipPreflight) {
                const auto report = make_native_sip_preflight_report();
                if (!g_smoke_mode)
                    MessageBoxW(window, report.c_str(),
                                L"Auditoria pre-ativacao SIP - AXE Voice LAB",
                                MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            if (control == kReadinessButton) {
                refresh_runtime_banner(g_baresip_process.poll());
                const std::wstring report = make_readiness_report();
                if (g_smoke_mode) {
                    g_readiness_requested_smoke =
                        report.find(L"CHAMADAS BLOQUEADAS") != std::wstring::npos &&
                        report.find(L"sem evidencia real de registro") != std::wstring::npos;
                } else {
                    MessageBoxW(window, report.c_str(),
                                L"Prontidao de chamadas - AXE Voice LAB",
                                MB_OK | MB_ICONINFORMATION);
                }
                return 0;
            }
            if (control == kHelpButton) {
                if (g_smoke_mode) g_help_requested_smoke = true;
                else MessageBoxW(window, axv::frontend_help_text(),
                                 L"Ajuda - AXE Voice (LAB)", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            if (control == kRefreshHeadsetsButton && g_page == 2) {
                if (!g_smoke_mode && !defer_route_while_preview_active(
                        window, {axv::AudioRouteChangeKind::Refresh, {}}))
                    refresh_audio_panel();
                return 0;
            }
            if (control == kSwapAudioRolesButton && g_page == 2) {
                const axv::AudioRouteChange route{axv::AudioRouteChangeKind::Swap, {}};
                if (!defer_route_while_preview_active(window, route))
                    apply_audio_route_change(window, route);
                return 0;
            }
            if (control == kShowAudio) {
                if (g_compact) {
                    g_compact = false;
                    apply_compact_layout(window);
                    save_ui_preferences();
                }
                show_page(window, 2);
                if (!g_smoke_mode && !defer_route_while_preview_active(
                        window, {axv::AudioRouteChangeKind::Refresh, {}}))
                    refresh_audio_panel();
                return 0;
            }
            if (control == kShowSettings) {
                if (g_compact) {
                    g_compact = false;
                    apply_compact_layout(window);
                    save_ui_preferences();
                }
                show_page(window, 1);
                return 0;
            }
            if (control == kCompact) {
                g_compact = !g_compact;
                if (g_compact) g_page = 0;
                apply_compact_layout(window);
                save_ui_preferences();
                return 0;
            }
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
            g_sip_session_guard.revoke(axv::RuntimeProcess::Stopped, g_native_event_mailbox, g_sip_projection);
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
        if (LOWORD(wparam) == kRefreshHeadsetsButton && HIWORD(wparam) == BN_CLICKED) {
            if (!g_smoke_mode && !defer_route_while_preview_active(
                    window, {axv::AudioRouteChangeKind::Refresh, {}}))
                refresh_audio_panel();
            return 0;
        }
        if ((LOWORD(wparam) == kTestRingtoneButton ||
             LOWORD(wparam) == kTestVoiceButton) && HIWORD(wparam) == BN_CLICKED) {
            // The user must click; neither account nor incoming SIP events play audio.
            const auto role = LOWORD(wparam) == kTestVoiceButton
                ? axv::AudioPreviewRole::Voice : axv::AudioPreviewRole::Ringtone;
            // A second click on the active role requests a cooperative stop.
            if (axv::preview_can_cancel(g_preview_running.load(),
                    g_preview_cancel.load(), g_smoke_mode, g_preview_role, role)) {
                g_preview_cancel.store(true);
                SetWindowTextW(g_headset_status,
                    L"Interrompendo teste local. Nao e chamada SIP.");
                update_audio_preview_buttons(window);
                return 0;
            }
            const auto target = axv::select_audio_preview_target(
                g_audio_outputs, g_selected_output_id, g_selected_ringtone_id, role);
            if (!g_smoke_mode && g_page == 2 && !g_preview_running.load() &&
                !g_preview_pending && target.available) {
                if (g_preview_worker.joinable()) g_preview_worker.join();
                g_preview_cancel.store(false);
                g_preview_running.store(true);
                g_preview_result.store(-1);
                g_preview_pending = true;
                g_preview_role = role;
                update_audio_preview_buttons(window);
                try {
                    std::wstring id = target.endpoint_id;
                    const auto level = role == axv::AudioPreviewRole::Voice
                        ? g_voice_level : g_ring_level; // Snapshot at click.
                    g_preview_worker = std::thread([id = std::move(id), level] {
                        const auto result = axv::play_selected_ringtone_preview(
                            id, &g_preview_cancel, level);
                        g_preview_result.store(static_cast<int>(result));
                        g_preview_running.store(false);
                    });
                    SetWindowTextW(g_headset_status, role == axv::AudioPreviewRole::Voice
                        ? L"Testando saida de VOZ (tom local, sem SIP)..."
                        : L"Testando saida de TOQUE (tom local, sem SIP)...");
                } catch (...) {
                    g_preview_running.store(false);
                    g_preview_pending = false;
                    SetWindowTextW(g_headset_status,
                        L"Falha ao iniciar o teste local de audio.");
                    update_audio_preview_buttons(window);
                }
            }
            return 0;
        }
        if (HIWORD(wparam) == CBN_SELCHANGE) {
            const int id = LOWORD(wparam);
            if (id == kVoicePreviewLevel || id == kRingPreviewLevel) {
                HWND combo = id == kVoicePreviewLevel ? g_voice_level_combo :
                             g_ring_level_combo;
                std::uint8_t& level = id == kVoicePreviewLevel ?
                                     g_voice_level : g_ring_level;
                const int selected = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
                level = axv::preview_level_for_index(selected, level);
                SetWindowTextW(g_headset_status,
                    L"Volume do proximo teste atualizado. Sem alterar Windows ou SIP.");
                return 0;
            }
            if (id == kInputAudioCombo || id == kOutputAudioCombo ||
                id == kRingtoneAudioCombo) {
                HWND combo = id == kInputAudioCombo ? g_input_audio_combo :
                             id == kOutputAudioCombo ? g_output_audio_combo : g_ringtone_audio_combo;
                const auto& devices = id == kInputAudioCombo ? g_audio_inputs : g_audio_outputs;
                const LRESULT index = SendMessageW(combo, CB_GETCURSEL, 0, 0);
                if (index >= 0 && static_cast<std::size_t>(index) < devices.size()) {
                    const auto kind = id == kInputAudioCombo ? axv::AudioRouteChangeKind::Microphone :
                        id == kOutputAudioCombo ? axv::AudioRouteChangeKind::Voice :
                        axv::AudioRouteChangeKind::Ringtone;
                    const axv::AudioRouteChange route{kind,
                        devices[static_cast<std::size_t>(index)].id};
                    if (defer_route_while_preview_active(window, route)) {
                        const auto& current = id == kInputAudioCombo ? g_selected_input_id :
                            id == kOutputAudioCombo ? g_selected_output_id : g_selected_ringtone_id;
                        restore_audio_combo_selection(combo, devices, current);
                    } else {
                        apply_audio_route_change(window, route);
                    }
                }
                return 0;
            }
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
            // Local syntax only, never invokes configure() or register_account().
            refresh_account_feedback();
            return 0;
        }
        if (LOWORD(wparam) == kCallButton && HIWORD(wparam) == BN_CLICKED) {
            // Never trust a previously enabled button. Re-poll process and
            // provenance on every user action, including synthetic WM_COMMAND.
            wchar_t number[256] = {};
            GetWindowTextW(g_number, number, 256);
            const auto& observed = g_sip_session_guard.observe(
                observed_process(g_baresip_process.poll(), g_shutdown.pending()),
                axv::SipEvidence::None, g_native_event_mailbox, g_sip_projection);
            const auto permission = axv::authorize_dial(
                observed, observed.session, g_sip_selection, number);
            if (!permission.allowed()) {
                SetWindowTextW(g_status, axv::dial_denial_text(permission.reason));
            } else if (!g_sip_selection.engine->start_call(permission.normalized_destination)) {
                SetWindowTextW(g_status, g_sip_selection.engine->diagnostic().c_str());
            }
            return 0;
        }
        break;
    case WM_DPICHANGED: {
        g_scroll_offset=0;
        const unsigned next=axv::normalized_dpi(LOWORD(wparam));
        if(next!=g_dpi) {
            g_dpi=next;
            rebuild_fonts();
            for(const auto& child:g_layout)
                position_logical(child.handle, child.logical.x, child.logical.y,
                                 child.logical.width, child.logical.height);
        }
        if(lparam) {
            const auto* suggested=reinterpret_cast<const RECT*>(lparam);
            SetWindowPos(window,nullptr,suggested->left,suggested->top,0,0,
                         SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        }
        apply_compact_layout(window);
        sync_viewport_scroll(window);
        InvalidateRect(window,nullptr,TRUE);
        EnumChildWindows(window,repaint_child,0);
        return 0;
    }
    case WM_VSCROLL: {
        if (!g_controls_ready) return 0;
        RECT client{};
        GetClientRect(window,&client);
        const int page=std::max(1,client.bottom-client.top);
        int desired=g_scroll_offset;
        switch (LOWORD(wparam)) {
        case SB_TOP: desired=0; break;
        case SB_BOTTOM: desired=g_scroll_max; break;
        case SB_LINEUP: desired-=axv::dpi_pixels(42,g_dpi); break;
        case SB_LINEDOWN: desired+=axv::dpi_pixels(42,g_dpi); break;
        case SB_PAGEUP: desired-=page; break;
        case SB_PAGEDOWN: desired+=page; break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK: {
            SCROLLINFO info{};
            info.cbSize=sizeof(info);
            info.fMask=SIF_TRACKPOS;
            GetScrollInfo(window,SB_VERT,&info);
            desired=info.nTrackPos;
            break;
        }
        default: return 0;
        }
        set_viewport_scroll(window,desired);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        g_wheel_remainder+=GET_WHEEL_DELTA_WPARAM(wparam);
        const int steps=g_wheel_remainder/WHEEL_DELTA;
        g_wheel_remainder%=WHEEL_DELTA;
        if (steps) set_viewport_scroll(window,g_scroll_offset-
            steps*axv::dpi_pixels(48,g_dpi));
        return 0;
    }
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED) sync_viewport_scroll(window);
        if (wparam == SIZE_MINIMIZED && install_tray(window)) {
            ShowWindow(window, SW_HIDE);
            return 0;
        }
        break;
    case kTrayCallback:
        // NOTIFYICON_VERSION_4 puts the event in LOWORD(lParam).
        if (LOWORD(lparam) == WM_LBUTTONDBLCLK) {
            restore_tray(window);
            return 0;
        }
        if (LOWORD(lparam) == WM_RBUTTONUP || LOWORD(lparam) == WM_CONTEXTMENU) {
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
                if (choice == kTrayExit) {
                    g_explicit_exit = true;
                    SendMessageW(window, WM_CLOSE, 0, 0);
                }
                PostMessageW(window, WM_NULL, 0, 0);
            }
            return 0;
        }
        break;
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
        // Windows Accessibility/High Contrast overrides the stored visual theme.
        // Neither SIP state nor preferences change on accessibility events.
        read_windows_contrast();
        refresh_brushes();
        InvalidateRect(window, nullptr, TRUE);
        EnumChildWindows(window, repaint_child, 0);
        return 0;
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
        const HWND edit = reinterpret_cast<HWND>(lparam);
        const bool invalid = (edit == g_number && g_destination_invalid) ||
                             (edit == g_server && g_account_invalid_server) ||
                             (edit == g_user && g_account_invalid_user) ||
                             (edit == g_port && g_account_invalid_port);
        SetTextColor(dc, text_color());
        SetBkColor(dc, invalid ? invalid_field_color() : field_color());
        return reinterpret_cast<LRESULT>(invalid ? g_invalid_field_brush : g_field_brush);
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
        draw_card(dc,logical_rect(24,151,414,198),panel_color(),16);
        draw_card(dc,g_compact?logical_rect(24,206,414,289)
                               :logical_rect(24,206,414,742),panel_color(),18);
        HPEN rule = CreatePen(PS_SOLID, 1, line_color());
        HGDIOBJ old = SelectObject(dc, rule);
        const int divider_y = g_compact ? 294 : 747;
        MoveToEx(dc,axv::dpi_pixels(24,g_dpi),axv::dpi_pixels(divider_y,g_dpi)-g_scroll_offset,nullptr);
        LineTo(dc,axv::dpi_pixels(414,g_dpi),axv::dpi_pixels(divider_y,g_dpi)-g_scroll_offset);
        SelectObject(dc, old);
        DeleteObject(rule);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_TIMER:
        if (wparam == kProcessTimer) {
            if (!g_smoke_mode && g_audio_hotplug.take_if_due(
                    GetTickCount64(), g_page == 2 && IsWindowVisible(window) &&
                    !IsIconic(window))) {
                // Stop local preview cooperatively before touching its route.
                const axv::AudioRouteChange update{
                    axv::AudioRouteChangeKind::Refresh, {}};
                if (!defer_route_while_preview_active(window, update))
                    refresh_audio_panel();
            }
            if (g_preview_pending && !g_preview_running.load()) {
                if (g_preview_worker.joinable()) g_preview_worker.join();
                g_preview_pending = false;
                const auto result = static_cast<axv::RingPreviewResult>(
                    g_preview_result.load());
                const bool voice = g_preview_role == axv::AudioPreviewRole::Voice;
                SetWindowTextW(g_headset_status,
                    result == axv::RingPreviewResult::Cancelled
                    ? (voice ? L"Teste local de VOZ interrompido."
                             : L"Teste local de TOQUE interrompido.")
                    : result == axv::RingPreviewResult::Played
                    ? (voice ? L"Teste local de VOZ enviado. Nao e chamada SIP."
                             : L"Teste local de TOQUE enviado. Nao e chamada SIP.")
                    : (voice ? L"Falha ao testar saida de VOZ ou formato de audio."
                             : L"Falha ao testar saida de TOQUE ou formato de audio."));
                update_audio_preview_buttons(window);
                // join() above is the STOP acknowledgement for the local WASAPI worker.
                const auto route = g_audio_route_fence.take_after_stop_ack(true);
                if (route.kind != axv::AudioRouteChangeKind::None)
                    apply_audio_route_change(window, route);
            }
            refresh_runtime_banner(g_baresip_process.poll());
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
        if (g_close_to_tray && !g_explicit_exit && !g_smoke_mode && install_tray(window)) {
            ShowWindow(window, SW_HIDE);
            return 0;
        }
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
        (void)g_incoming_ring_coordinator.revoke(g_incoming_ring_sink);
        // Stop cooperatively before waiting; worker never posts to a dead HWND.
        g_preview_cancel.store(true);
        if (g_preview_worker.joinable()) g_preview_worker.join();
        g_audio_route_fence.clear();
        g_sip_session_guard.revoke(axv::RuntimeProcess::Stopped, g_native_event_mailbox, g_sip_projection);
        KillTimer(window, kProcessTimer);
        remove_tray(window);
        g_layout.clear();
        if (g_font_regular) DeleteObject(g_font_regular);
        if (g_font_title) DeleteObject(g_font_title);
        if (g_font_small) DeleteObject(g_font_small);
        if (g_font_digit) DeleteObject(g_font_digit);
        if (g_background_brush) DeleteObject(g_background_brush);
        if (g_field_brush) DeleteObject(g_field_brush);
        if (g_invalid_field_brush) DeleteObject(g_invalid_field_brush);
        // Normally stopped through WM_CLOSE. Forced fallback on external destruction.
        g_baresip_process.stop();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command_line, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    g_dpi=axv::normalized_dpi(GetDpiForSystem());
    const wchar_t* className = L"AXEVoiceDesktopWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc)) return 1;
    g_smoke_mode = command_line && (std::wcscmp(command_line, L"--ui-smoke") == 0 ||
                                      std::wcscmp(command_line, L"--ui-blackbox") == 0);
    HWND window = CreateWindowExW(0, className, L"AXE Voice  |  Desktop Lab v0.139",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        axv::dpi_pixels(kWindowWidth,g_dpi)+GetSystemMetricsForDpi(SM_CXVSCROLL,g_dpi),
        axv::dpi_pixels(kFullHeight,g_dpi),
        nullptr, nullptr, instance, nullptr);
    if (!window) return 2;
    if (g_smoke_mode && command_line &&
        std::wcscmp(command_line, L"--ui-smoke") == 0) {
        // Force a consistent baseline without changing saved HKCU settings.
        if (g_compact) { g_compact = false; apply_compact_layout(window); }
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
        SetWindowTextW(g_server, L"pbx.example.org");
        SetWindowTextW(g_user, L"1001");
        SetWindowTextW(g_port, L"65536");
        wchar_t account_message[160] = {};
        GetWindowTextW(g_status, account_message, 160);
        const bool account_invalid_ok = g_account_invalid_port &&
            std::wstring_view(account_message).find(L"intervalo") != std::wstring_view::npos;
        SetWindowTextW(g_port, L"5060");
        GetWindowTextW(g_status, account_message, 160);
        const bool account_valid_ok = !g_account_invalid_port &&
            std::wstring_view(account_message).find(L"Registro SIP nao realizado") != std::wstring_view::npos &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        ok = ok && account_invalid_ok && account_valid_ok;
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kShowDial, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kShowDial)));
        ok = ok && (GetWindowLongW(g_number, GWL_STYLE) & WS_VISIBLE) &&
             !(GetWindowLongW(g_server, GWL_STYLE) & WS_VISIBLE);
        wchar_t engine_banner[160] = {};
        GetWindowTextW(g_runtime_banner, engine_banner, 160);
        ok = ok && std::wstring_view(engine_banner).find(L"NAO REGISTRADO") != std::wstring_view::npos;
        const bool baseline_ok = ok;
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kHelpButton, 1), 0);
        const bool help_ok = g_help_requested_smoke &&
                             GetDlgItem(window, kHelpButton) != nullptr;
        ok = ok && help_ok;
        // EN_CHANGE must validate keyboard/paste input, without enabling SIP calls.
        wchar_t feedback_message[160]{};
        SetWindowTextW(g_number, L"ana@bad host");
        GetWindowTextW(g_status, feedback_message, 160);
        const bool invalid_feedback_ok = g_destination_invalid &&
            std::wstring_view(feedback_message).find(L"espacos") != std::wstring_view::npos;
        SetWindowTextW(g_number, L"1001");
        GetWindowTextW(g_status, feedback_message, 160);
        const bool valid_feedback_ok = !g_destination_invalid &&
            std::wstring_view(feedback_message).find(L"Ramal valido") != std::wstring_view::npos &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        // Direct WM_COMMAND must fail closed even if the visual button is
        // disabled or a caller bypasses the normal user interaction path.
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kCallButton, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kCallButton)));
        wchar_t denied_message[160]{};
        GetWindowTextW(g_status, denied_message, 160);
        const bool direct_command_denied =
            std::wstring_view(denied_message).find(L"AXV-SIP-003") != std::wstring_view::npos &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        SetWindowTextW(g_number, L"1234");
        SendMessageW(g_number, EM_SETSEL, 2, 2);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kDigitStart + 4, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kDigitStart + 4)));
        wchar_t edited[32] = {};
        GetWindowTextW(g_number, edited, 32);
        const bool insert_at_caret_ok = std::wstring_view(edited) == L"12534";
        SendMessageW(g_number, EM_SETSEL, 1, 4);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kBackspace, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kBackspace)));
        GetWindowTextW(g_number, edited, 32);
        const bool selected_delete_ok = std::wstring_view(edited) == L"14";
        ok = ok && insert_at_caret_ok && selected_delete_ok;
        SetWindowTextW(g_number, L"");
        GetWindowTextW(g_status, feedback_message, 160);
        const bool empty_feedback_ok = !g_destination_invalid &&
            std::wstring_view(feedback_message).find(L"Digite") != std::wstring_view::npos;
        ok = ok && invalid_feedback_ok && valid_feedback_ok && empty_feedback_ok && direct_command_denied;
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kCompact, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kCompact)));
        RECT bounds = {};
        GetWindowRect(window, &bounds);
        const bool compact_ok = g_compact &&
            (bounds.bottom - bounds.top == axv::dpi_pixels(kCompactHeight,g_dpi)) &&
            (GetWindowLongW(g_number, GWL_STYLE) & WS_VISIBLE) &&
            !(GetWindowLongW(GetDlgItem(window, kDigitStart), GWL_STYLE) & WS_VISIBLE) &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        ok = ok && compact_ok;
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kShowSettings, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kShowSettings)));
        GetWindowRect(window, &bounds);
        const bool restored_ok = !g_compact && g_page == 1 &&
            // CI desktop work area may clamp the requested 842px window height.
            (bounds.bottom - bounds.top > axv::dpi_pixels(kCompactHeight+200,g_dpi)) &&
            (GetWindowLongW(g_server, GWL_STYLE) & WS_VISIBLE);
        ok = ok && restored_ok;
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kShowAudio, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kShowAudio)));
        const bool audio_ok = !g_incoming_ring_coordinator.playing() &&
            !g_incoming_ring_coordinator.stop_pending() && g_page == 2 &&
            (GetWindowLongW(g_input_audio_combo, GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(g_output_audio_combo, GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(g_ringtone_audio_combo, GWL_STYLE) & WS_VISIBLE) &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton)) &&
            !IsWindowEnabled(g_input_audio_combo) &&
            !IsWindowEnabled(g_ringtone_audio_combo) &&
            GetDlgItem(window, kTestRingtoneButton) &&
            GetDlgItem(window, kTestVoiceButton) &&
            g_voice_level_combo && g_ring_level_combo &&
            (GetWindowLongW(g_voice_level_combo, GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(g_ring_level_combo, GWL_STYLE) & WS_VISIBLE) &&
            !IsWindowEnabled(GetDlgItem(window, kTestRingtoneButton)) &&
            !IsWindowEnabled(GetDlgItem(window, kTestVoiceButton)) &&
            !g_preview_worker.joinable() &&
            GetDlgItem(window, kSwapAudioRolesButton) &&
            (GetWindowLongW(GetDlgItem(window, kSwapAudioRolesButton), GWL_STYLE) & WS_VISIBLE) &&
            !IsWindowEnabled(GetDlgItem(window, kSwapAudioRolesButton)); // No hardware inspected in smoke.
        // Test independent role gain selection without touching Windows mixer or audio.
        SendMessageW(g_voice_level_combo, CB_SETCURSEL, 0, 0);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kVoicePreviewLevel, CBN_SELCHANGE),
                     reinterpret_cast<LPARAM>(g_voice_level_combo));
        SendMessageW(g_ring_level_combo, CB_SETCURSEL, 3, 0);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kRingPreviewLevel, CBN_SELCHANGE),
                     reinterpret_cast<LPARAM>(g_ring_level_combo));
        const bool gain_ok = g_voice_level == 25 && g_ring_level == 100 &&
            !g_preview_running.load() &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        // Exercise real Win32 controls with synthetic outputs, never probing devices or SIP.
        g_audio_outputs = {
            {L"test-headset", L"Headset", true, false},
            {L"test-speaker", L"Computer", false, true}
        };
        g_selected_output_id = L"test-headset";
        g_selected_ringtone_id = L"test-speaker";
        const std::wstring mic_before = g_selected_input_id;
        fill_audio_combo(g_output_audio_combo, g_audio_outputs, g_selected_output_id);
        fill_audio_combo(g_ringtone_audio_combo, g_audio_outputs, g_selected_ringtone_id, true);
        update_swap_button();
        const bool can_swap_in_smoke = IsWindowEnabled(GetDlgItem(window, kSwapAudioRolesButton));
        SendMessageW(window, WM_COMMAND,
            MAKEWPARAM(kSwapAudioRolesButton, BN_CLICKED),
            reinterpret_cast<LPARAM>(GetDlgItem(window, kSwapAudioRolesButton)));
        const auto voice_preview = axv::select_audio_preview_target(
            g_audio_outputs, g_selected_output_id, g_selected_ringtone_id,
            axv::AudioPreviewRole::Voice);
        const auto ring_preview = axv::select_audio_preview_target(
            g_audio_outputs, g_selected_output_id, g_selected_ringtone_id,
            axv::AudioPreviewRole::Ringtone);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kTestVoiceButton, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window,kTestVoiceButton)));
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kTestRingtoneButton, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window,kTestRingtoneButton)));
        const bool preview_safe_smoke = voice_preview.available && ring_preview.available &&
            voice_preview.endpoint_id == L"test-speaker" &&
            ring_preview.endpoint_id == L"test-headset" &&
            !g_preview_worker.joinable() && !g_preview_running.load() &&
            !IsWindowEnabled(GetDlgItem(window,kCallButton));
        const bool swapped_in_smoke = can_swap_in_smoke && preview_safe_smoke &&
            g_selected_output_id == L"test-speaker" &&
            g_selected_ringtone_id == L"test-headset" &&
            g_selected_input_id == mic_before &&
            SendMessageW(g_output_audio_combo, CB_GETCURSEL, 0, 0) == 1 &&
            SendMessageW(g_ringtone_audio_combo, CB_GETCURSEL, 0, 0) == 0 &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        g_audio_outputs.clear();
        g_selected_output_id.clear();
        g_selected_ringtone_id.clear();
        fill_audio_combo(g_output_audio_combo, g_audio_outputs, g_selected_output_id);
        fill_audio_combo(g_ringtone_audio_combo, g_audio_outputs, g_selected_ringtone_id, true);
        update_swap_button();
        ok = ok && gain_ok && swapped_in_smoke &&
             g_voice_level == 25 && g_ring_level == 100 &&
             !IsWindowEnabled(GetDlgItem(window, kSwapAudioRolesButton)) &&
             !IsWindowEnabled(GetDlgItem(window, kTestVoiceButton)) &&
             !IsWindowEnabled(GetDlgItem(window, kTestRingtoneButton));
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kReadinessButton, 1), 0);
        const bool readiness_ok = g_readiness_requested_smoke &&
            GetDlgItem(window, kReadinessButton) &&
            (GetWindowLongW(GetDlgItem(window, kReadinessButton), GWL_STYLE) & WS_VISIBLE) &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        // Test the palette integration without modifying the host's accessibility settings.
        const bool original_contrast = g_high_contrast;
        g_high_contrast = true;
        const bool contrast_ok = background_color() == GetSysColor(COLOR_WINDOW) &&
            text_color() == GetSysColor(COLOR_WINDOWTEXT) &&
            accent_color() == GetSysColor(COLOR_HIGHLIGHT) &&
            ui_color(axv::UiRole::SelectedText) == GetSysColor(COLOR_HIGHLIGHTTEXT) &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        g_high_contrast = original_contrast;
        ok=ok && audio_ok && readiness_ok && contrast_ok;
        // DPI reflow is synthetic; no display settings, HKCU or SIP are touched.
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kShowDial,BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window,kShowDial)));
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(144,144),0);
        RECT target{};
        GetWindowRect(g_number,&target);
        MapWindowPoints(HWND_DESKTOP,window,reinterpret_cast<LPPOINT>(&target),2);
        const bool dpi_150_ok=g_dpi==144 &&
            target.left==axv::dpi_pixels(39,144) &&
            target.top==axv::dpi_pixels(272,144) &&
            target.right-target.left==axv::dpi_pixels(350,144) &&
            !IsWindowEnabled(GetDlgItem(window,kCallButton));
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window,kCompact)));
        GetWindowRect(window,&bounds);
        const bool compact_150_ok=g_compact &&
            bounds.bottom-bounds.top==axv::dpi_pixels(kCompactHeight,144);
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(192,192),0);
        GetWindowRect(window,&bounds);
        const bool compact_200_ok=g_dpi==192 && g_compact &&
            bounds.bottom-bounds.top==axv::dpi_pixels(kCompactHeight,192);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kShowSettings,BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window,kShowSettings)));
        const bool full_200_ok=!g_compact && g_page==1 &&
            !IsWindowEnabled(GetDlgItem(window,kCallButton));
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(96,96),0);
        ok=ok && dpi_150_ok && compact_150_ok && compact_200_ok &&
           full_200_ok && g_dpi==96 && !IsWindowEnabled(GetDlgItem(window,kCallButton));
        if (!ok) {
            std::fprintf(stderr, "UI_SMOKE_FAIL baseline=%d compact=%d restored=%d audio=%d feedback=%d/%d/%d mode=%d page=%d height=%ld\n",
                         baseline_ok ? 1 : 0, compact_ok ? 1 : 0, restored_ok ? 1 : 0,
                         audio_ok ? 1 : 0, invalid_feedback_ok ? 1 : 0,
                         valid_feedback_ok ? 1 : 0, empty_feedback_ok ? 1 : 0,
                         g_compact ? 1 : 0, g_page, bounds.bottom - bounds.top);
            std::fflush(stderr);
        }
        DestroyWindow(window);
        return ok ? 0 : 9;
    }
    const bool request_tray = command_line &&
        std::wcscmp(command_line, L"--tray") == 0;
    if (request_tray && install_tray(window)) {
        ShowWindow(window, SW_HIDE);
    } else {
        ShowWindow(window, show);
    }
    UpdateWindow(window);
    const ACCEL shortcuts[] = {
        {FVIRTKEY | FCONTROL, L'K', kShowDial},
        {FVIRTKEY | FCONTROL, L'L', kFocusNumber},
        {FVIRTKEY | FCONTROL, VK_OEM_COMMA, kShowSettings},
        {FVIRTKEY | FCONTROL, L'T', kTheme},
        {FVIRTKEY | FCONTROL, L'P', kPin},
        {FVIRTKEY | FCONTROL, L'M', kCompact},
        {FVIRTKEY | FCONTROL, L'1', kShowDial},
        {FVIRTKEY | FCONTROL, L'2', kShowSettings},
        {FVIRTKEY | FCONTROL, L'3', kShowAudio},
        {FVIRTKEY | FCONTROL, L'R', kRefreshHeadsetsButton},
        {FVIRTKEY | FCONTROL, L'G', kMoreButton},
        {FVIRTKEY | FCONTROL, L'D', kSaveFavorite},
        {FVIRTKEY | FCONTROL, L'F', kLoadFavorite},
        {FVIRTKEY | FCONTROL, L'H', kShowRecent},
        {FVIRTKEY | FCONTROL | FSHIFT, L'P', kPrivacyDial},
        {FVIRTKEY | FCONTROL | FSHIFT, L'R', kRecallLastDestination},
        {FVIRTKEY, VK_ESCAPE, kShowDial},
        {FVIRTKEY, VK_F1, kHelpButton},
        {FVIRTKEY, VK_F2, kReadinessButton},
        {FVIRTKEY, VK_F3, kSipPreflight},
        {FVIRTKEY, VK_F4, kSipVaultStatus},
        {FVIRTKEY, VK_F5, kOpenCredentialManager},
        {FVIRTKEY, VK_F6, kSipNextSteps},
        {FVIRTKEY, VK_F7, kRemoveSipVaultCredential}
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
