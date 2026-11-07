#include <windows.h>
#ifdef AXV_ENABLE_NATIVE_SIP
#include <wincred.h>
#endif
#include <shlobj.h>
#include <commdlg.h>
#include <mutex>
#include <shellapi.h>
#include <commctrl.h>
#include "faleweb_preferences_win32.h"
#include "baresip_profile.h"
#include "sip_address.h"
#include "sip_engine.h"
#include "sip_account.h"
#include "sip_status.h"
#include "sip_runtime_observation.h"
#include "essential_call_actions.h"
#include "baresip_event_mailbox.h"
#include "sip_event_projection.h"
#include "sip_runtime_session_guard.h"
#include "dial_authorization.h"
#include "call_command_authorization.h"
#include "local_cti_server_win32.h"
#include "local_cti_contract.h"
#include "call_policy.h"
#include "sip_diagnostics.h"
#include "sip_registration_policy.h"
#include "sip_security_policy.h"
#include "sip_engine_factory.h"
#include "baresip_probe.h"
#include "baresip_modules.h"
#include "audio_devices.h"
#include "audio_inventory_async.h"
#include "audio_inventory_retry.h"
#include "audio_endpoint_watcher_win32.h"
#include "audio_device_change_policy.h"
#include "audio_panel_selection.h"
#include "audio_selection_feedback.h"
#include "native_audio_routes.h"
#include "audio_routing_preview.h"
#include "audio_output_preview.h"
#include "audio_test_tone_win32.h"
#include "microphone_test.h"
#include "microphone_test_win32.h"
#include "incoming_ringtone_policy.h"
#include "preferences_incoming_policy.h"
#ifdef AXV_ENABLE_NATIVE_SIP
#include "native_install_guard.h"
#include "voice_application_service.h"
#include "contact_dialog.h"
#include "contact_ui_actions.h"
#include "incoming_ringtone_player.h"
#include "native_registration_ui.h"
#include "recent_destination_ui.h"
#include "native_dtmf.h"
#include "native_call_duration.h"
#include "call_duration_format.h"
#endif
#include "windows_process.h"
#include "shutdown_policy.h"
#include <string>
#include <string_view>
#include <cstdio>

#include "dialpad_model.h"
#include "destination_feedback.h"
#include "account_form_feedback.h"
#include "ui_help.h"
#include "call_readiness.h"
#include "diagnostic_export.h"
#include "diagnostic_clipboard_win32.h"
#include "ui_contrast_policy.h"
#include "ui_dpi_policy.h"
#include "ui_rounded_geometry.h"
#include "ui_viewport_policy.h"
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <cwchar>
#include <iterator>
#include <optional>


namespace {
#ifdef AXV_ENABLE_NATIVE_SIP
// Retain through process exit, including detached audio probes after owner teardown.
axv::NativeInstallGuard g_native_install_guard{axv::NativeInstallGuardLifetime::Process};
constexpr int kPasswordEdit=1016,kCaEdit=1017,kAnswerNative=1018,kRememberSipPassword=1020;
constexpr wchar_t kSipCredentialTarget[]=L"AXEVoice/SIP/DefaultAccount";
HWND g_password=nullptr,g_ca_file=nullptr,g_remember_password=nullptr;

axv::NativeCallMediaState g_displayed_media;
constexpr int kCallDurationLabel = 2503;
constexpr int kRingtoneStatusLabel = 2504;
HWND g_call_duration_label = nullptr;
HWND g_ringtone_status_label = nullptr;
axv::NativeCallDuration g_call_duration;
axv::RecentDestinations g_recent_destinations;
bool g_incoming_ringtone_muted=false; // Local preference for this execution only.
axv::LocalCtiServer g_cti_server;

#endif
constexpr int kCallButton = 1001;
constexpr int kAnswerNativeColorId = 1018; // Native answer control ID.
constexpr int kNumberEdit = 1002;
constexpr int kValidateAccount = 1003;
constexpr int kServerEdit = 1004;
constexpr int kUserEdit = 1005;
constexpr int kPortEdit = 1006;
constexpr int kTlsCheckbox = 1007;
constexpr int kProbeButton = 1008;
constexpr int kAudioDevicesButton = 1009;
constexpr int kRefreshHeadsetsButton = 1013;
constexpr int kTestRingtoneButton = 1014;
constexpr int kTestConversationButton = 1015;
constexpr int kTestMicrophoneButton = 1019;
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
constexpr int kCopyStatusButton = 2009;
constexpr int kDtmfButton = 2010;
constexpr int kRecentDestinationsButton = 2011;
constexpr int kIncomingRingtoneMuteButton = 2012;
constexpr int kContactsButton = 2013;
constexpr int kFalewebPrefsButton = 2015;
constexpr int kFalewebActionsButton = 2016;
constexpr int kShowSipMonitor = 2017;
constexpr int kInputAudioCombo = 2401;
constexpr int kOutputAudioCombo = 2402;
constexpr int kRingtoneAudioCombo = 2403;
constexpr int kRingtonePresetCombo = 2404;
constexpr int kRingtoneChooseMp3 = 2405;
// Stable Win32 UI Automation IDs for real status labels.
constexpr int kFeedbackLabel = 2501;
constexpr int kRuntimeStatusLabel = 2502;
constexpr int kWindowWidth = 438;
constexpr int kFalewebWidth=254;
constexpr int kFalewebHeight=475;
// The normal Essencial view is 80% of the approved reference canvas.
constexpr int kEssentialScalePercent=80;
constexpr int kEssentialWidth=355;
constexpr int kEssentialHeight=602;
unsigned g_dpi = axv::kBaseDpi;
#ifdef AXV_ENABLE_NATIVE_SIP
constexpr int kFullHeight = 886;
constexpr int kFullScrollContentEnd = 737;
constexpr int kFullFeedbackY = 806;
#else
constexpr int kFullHeight = 842;
constexpr int kFullScrollContentEnd = 737;
constexpr int kFullFeedbackY = 762;
#endif
constexpr int kMiniScrollContentEnd = 298;
constexpr int kFooterHeight = 48;
int g_scroll_offset = 0;
int g_scroll_max = 0;
int g_wheel_remainder = 0;
bool g_controls_ready = false;
bool g_scrollbar_updating = false;
bool g_scroll_message_queued = false;
int g_pending_scroll_offset = 0;
constexpr int kCompactHeight = 395; // Keep dial input clear of fixed explanation footer.
constexpr int kDigitStart = 2100;
constexpr int kBackspace = 2201;
constexpr int kClear = 2202;
constexpr int kMuteFuture = 2301;
constexpr int kHoldFuture = 2302;
constexpr int kTransferFuture = 2303;
constexpr int kHangupFuture = 2304;
constexpr int kFocusNumber = 2305;
constexpr UINT kTrayCallback = WM_APP + 41;
constexpr UINT kAsyncScrollMessage = WM_APP + 43;
constexpr UINT kAsyncCtiMessage = WM_APP + 44;
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayOpen = 3001;
constexpr UINT kTrayExit = 3002;
HWND g_number = nullptr;
HWND g_number_label = nullptr;
HWND g_status = nullptr;
HWND g_footer_panel = nullptr;
HWND g_server = nullptr;
HWND g_user = nullptr;
HWND g_port = nullptr;
HWND g_tls = nullptr;
HWND g_audio_status = nullptr;
HWND g_process_status = nullptr;
#ifdef AXV_ENABLE_NATIVE_SIP
// Windows Credential Manager stores the secret for the current Windows user.
// Account identity is checked on every read to prevent use with another SIP account.
std::wstring sip_credential_identity() {
    return axv::pref_edit_text(g_server)+L"\n"+axv::pref_edit_text(g_user);
}
bool sip_credential_exists() {
    PCREDENTIALW credential=nullptr;
    if(!CredReadW(kSipCredentialTarget,CRED_TYPE_GENERIC,0,&credential))return false;
    const auto identity=sip_credential_identity();
    const bool matches=credential->UserName && !identity.empty() &&
                       identity==credential->UserName;
    CredFree(credential);
    return matches;
}
// Never delete another account's stored credential just because the current
// form was changed. Return false only when Windows reports a deletion error.
bool forget_sip_credential() {
    if(!sip_credential_exists())return true;
    if(CredDeleteW(kSipCredentialTarget,CRED_TYPE_GENERIC,0))return true;
    return GetLastError()==ERROR_NOT_FOUND;
}
bool save_sip_credential(const std::wstring& secret) {
    if(secret.empty() || secret.size()>256 || !g_server || !g_user)return false;
    const auto identity=sip_credential_identity();
    if(identity.size()<3 || identity.front()==L'\n' || identity.back()==L'\n')return false;
    CREDENTIALW entry{};
    entry.Type=CRED_TYPE_GENERIC;
    entry.TargetName=const_cast<LPWSTR>(kSipCredentialTarget);
    entry.UserName=const_cast<LPWSTR>(identity.c_str());
    entry.CredentialBlob=reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(secret.data()));
    entry.CredentialBlobSize=static_cast<DWORD>(secret.size()*sizeof(wchar_t));
    entry.Persist=CRED_PERSIST_LOCAL_MACHINE;
    return CredWriteW(&entry,0)!=FALSE;
}
std::wstring load_sip_credential() {
    PCREDENTIALW credential=nullptr;
    if(!CredReadW(kSipCredentialTarget,CRED_TYPE_GENERIC,0,&credential))return {};
    std::wstring secret;
    if(credential->UserName &&
       sip_credential_identity()==credential->UserName &&
       credential->CredentialBlob &&
       credential->CredentialBlobSize>0 &&
       credential->CredentialBlobSize<=256*sizeof(wchar_t) &&
       credential->CredentialBlobSize%sizeof(wchar_t)==0) {
        const auto* data=reinterpret_cast<const wchar_t*>(credential->CredentialBlob);
        secret.assign(data,data+credential->CredentialBlobSize/sizeof(wchar_t));
    }
    CredFree(credential);
    return secret;
}
#endif
HWND g_runtime_banner = nullptr;
HWND g_input_audio_combo = nullptr;
HWND g_output_audio_combo = nullptr;
HWND g_ringtone_audio_combo = nullptr;
HWND g_ringtone_preset_combo = nullptr;
std::atomic<int> g_ringtone_preset{2};
axv::NativeCodecMode g_codec_mode=axv::NativeCodecMode::Automatic;
std::mutex g_ringtone_mp3_mutex;
std::wstring g_ringtone_custom_mp3;
HWND g_headset_status = nullptr;
std::vector<axv::AudioEndpoint> g_audio_inputs;
std::vector<axv::AudioEndpoint> g_audio_outputs;
std::wstring g_selected_input_id;
std::wstring g_selected_output_id;
std::wstring g_selected_ringtone_id;
bool g_audio_inventory_current = false;
axv::AudioInventoryAsync g_audio_worker{axv::inspect_windows_audio_devices};
axv::AudioInventoryRetry g_audio_retry;
axv::AudioEndpointWatcher g_audio_endpoint_watcher;
axv::AudioDeviceChangePolicy g_audio_change_policy;
axv::AudioInventoryAsync g_settings_audio_worker{axv::inspect_windows_audio_devices};
axv::AudioInventoryRetry g_settings_audio_retry;
axv::TonePreviewPlayer g_tone_preview;
axv::TonePreviewState g_last_tone_state = axv::TonePreviewState::Idle;
bool g_preview_is_conversation = false;
axv::MicrophoneTestWorker g_microphone_test;
axv::MicrophoneTestState g_last_microphone_state = axv::MicrophoneTestState::Idle;
#ifdef AXV_ENABLE_NATIVE_SIP
axv::IncomingRingtonePolicy g_ring_policy;
axv::NativeDtmfMode g_dtmf_mode;
axv::IncomingRingtonePlayer g_ring_player{[](const std::wstring& id,const std::atomic<bool>& cancel){
    std::wstring mp3;
    {std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);mp3=g_ringtone_custom_mp3;}
    return axv::play_windows_ringtone(id,&cancel,true,g_ringtone_preset.load(),mp3);
}};
axv::SipRuntimeObservation g_displayed_sip;
bool g_ring_error_visible=false;
std::uint64_t g_last_incoming_visual_session=0;
std::uint64_t g_last_incoming_visual_call=0;
#endif
axv::WindowsProcess g_baresip_process;
axv::ProcessStatus g_last_process_state = axv::ProcessStatus::Stopped;
axv::ShutdownDeadline g_shutdown;
bool g_close_requested = false;
constexpr std::uint64_t kGracePeriodMs = 1500;
axv::EngineSelection g_sip_selection = axv::create_sip_engine();
#ifdef AXV_ENABLE_NATIVE_SIP
axv::BaresipSipEngine& native_engine() {
    return static_cast<axv::BaresipSipEngine&>(*g_sip_selection.engine);
}
axv::VoiceApplicationService<axv::BaresipSipEngine>& native_voice_service() {
    // Same Baresip instance as the existing Win32 application. No duplicate
    // engine or credentials; guards are checked here and by the SDK worker.
    static axv::VoiceApplicationService<axv::BaresipSipEngine> service(native_engine());
    return service;
}
#endif
// Prepared for future native callbacks; current laboratory build has no producer.
axv::BaresipEventMailbox g_native_event_mailbox;
axv::SipEventProjection g_sip_projection;
axv::SipRuntimeSessionGuard g_sip_session_guard;


bool g_dark = false; // Option 1: Essencial claro. Saved user theme still wins.
bool g_high_contrast = false;
axv::UiSystemColors g_system_colors{};
bool g_pinned = false;
bool g_compact = false;
bool g_faleweb_mode=false;
bool g_faleweb_smoke=false;
bool g_essential_smoke=false;
axv::EssentialCallActions g_essential_actions{};
axv::EssentialConnectionSignal g_essential_signal=axv::EssentialConnectionSignal::Offline;
HBRUSH g_essential_status_brush[3]{};
HWND g_brand_label=nullptr;
HWND g_faleweb_prefs_button=nullptr;
HWND g_faleweb_actions_button=nullptr;
HBRUSH g_faleweb_display_brush=nullptr;
bool g_smoke_mode = false;
constexpr UINT kExternalClickToCallMessage=WM_APP+42;
constexpr ULONG_PTR kExternalClickDataId=0x41585643;
std::optional<std::wstring> g_external_click_uri;
bool g_help_requested_smoke = false;
bool g_readiness_requested_smoke = false;
bool g_copy_requested_smoke = false;
bool g_destination_invalid = false;
bool g_account_invalid_server = false;
bool g_account_invalid_user = false;
bool g_account_invalid_port = false;
bool g_tray_visible = false;
constexpr wchar_t kPrefsKey[] = L"Software\\AXEVoice\\Desktop";
#ifdef AXV_ENABLE_NATIVE_SIP
// Persist only non-secret account identifiers, and only after opting into
// remembering the SIP password. Password bytes always remain in Credential Manager.
bool save_sip_account_identifiers() {
    const auto server=axv::pref_edit_text(g_server);
    const auto user=axv::pref_edit_text(g_user);
    const auto port=axv::pref_edit_text(g_port);
    if(server.empty() || user.empty() || port.empty())return false;
    HKEY key=nullptr;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,kPrefsKey,0,nullptr,0,KEY_SET_VALUE,
                       nullptr,&key,nullptr)!=ERROR_SUCCESS)return false;
    const auto write=[&](const wchar_t* name,const std::wstring& value){
        return RegSetValueExW(key,name,0,REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            static_cast<DWORD>((value.size()+1)*sizeof(wchar_t)))==ERROR_SUCCESS;
    };
    const bool ok=write(L"SipServer",server) && write(L"SipUser",user) && write(L"SipPort",port);
    RegCloseKey(key);
    return ok;
}
void clear_sip_account_identifiers(){
    HKEY key=nullptr;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,kPrefsKey,0,KEY_SET_VALUE,&key)!=ERROR_SUCCESS)return;
    RegDeleteValueW(key,L"SipServer");
    RegDeleteValueW(key,L"SipUser");
    RegDeleteValueW(key,L"SipPort");
    RegCloseKey(key);
}
void restore_sip_account_identifiers(){
    const auto restore=[](const wchar_t* name,HWND edit,DWORD max_chars){
        if(!edit)return;
        std::wstring buffer(static_cast<size_t>(max_chars)+1,L'\0');
        DWORD bytes=static_cast<DWORD>(buffer.size()*sizeof(wchar_t));
        if(RegGetValueW(HKEY_CURRENT_USER,kPrefsKey,name,RRF_RT_REG_SZ,nullptr,
                        buffer.data(),&bytes)==ERROR_SUCCESS)
            SetWindowTextW(edit,buffer.c_str());
    };
    restore(L"SipServer",g_server,255);
    restore(L"SipUser",g_user,128);
    restore(L"SipPort",g_port,20);
}
#endif
int g_page = 0; // 0 = dial, 1 = settings, 2 = audio, 3 = monitor
std::vector<HWND> g_dial_controls;
std::vector<HWND> g_monitor_controls;
HWND g_monitor_events = nullptr;
std::wstring g_monitor_text;
axv::SipRuntimeObservation g_monitor_last{};
bool g_monitor_has_last = false;
std::vector<HWND> g_settings_controls;
std::vector<HWND> g_audio_controls;
enum class UiFontKind { Regular, Title, Small, Digit };
struct ChildLayout { HWND handle; axv::DpiBox logical; UiFontKind font; };
std::vector<ChildLayout> g_layout;
HFONT g_font_regular = nullptr;
HFONT g_font_title = nullptr;
HFONT g_font_small = nullptr;
HFONT g_font_digit = nullptr;
HFONT g_faleweb_digit_font=nullptr;
HFONT g_faleweb_letters_font=nullptr;
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
    if(RegGetValueW(HKEY_CURRENT_USER,kPrefsKey,L"AudioCodecMode",RRF_RT_REG_DWORD,
                    nullptr,&value,&size)==ERROR_SUCCESS &&
       axv::valid_native_codec_mode(static_cast<axv::NativeCodecMode>(value)))
        g_codec_mode=static_cast<axv::NativeCodecMode>(value);
    value = 2; size = sizeof(value);
    if(RegGetValueW(HKEY_CURRENT_USER,kPrefsKey,L"RingtonePreset",RRF_RT_REG_DWORD,
                    nullptr,&value,&size)==ERROR_SUCCESS && value<=5)
        g_ringtone_preset.store(static_cast<int>(value));
    wchar_t custom_path[2049]{};DWORD path_size=sizeof(custom_path);
    if(RegGetValueW(HKEY_CURRENT_USER,kPrefsKey,L"RingtoneMp3",RRF_RT_REG_SZ,
                    nullptr,custom_path,&path_size)==ERROR_SUCCESS){
        std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);
        g_ringtone_custom_mp3=custom_path;
    }
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
    RegSetValueExW(key, L"CompactMode", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&compact), sizeof(compact));
    RegSetValueExW(key, L"DarkTheme", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&theme), sizeof(theme));
    RegSetValueExW(key, L"AlwaysOnTop", 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&pinned), sizeof(pinned));
    const DWORD codec=static_cast<DWORD>(g_codec_mode);
    RegSetValueExW(key,L"AudioCodecMode",0,REG_DWORD,
                   reinterpret_cast<const BYTE*>(&codec),sizeof(codec));
    const DWORD preset=static_cast<DWORD>(g_ringtone_preset.load());
    RegSetValueExW(key,L"RingtonePreset",0,REG_DWORD,
                   reinterpret_cast<const BYTE*>(&preset),sizeof(preset));
    std::wstring mp3;
    {std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);mp3=g_ringtone_custom_mp3;}
    RegSetValueExW(key,L"RingtoneMp3",0,REG_SZ,
                   reinterpret_cast<const BYTE*>(mp3.c_str()),
                   static_cast<DWORD>((mp3.size()+1)*sizeof(wchar_t)));
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
    if(g_faleweb_display_brush)DeleteObject(g_faleweb_display_brush);
    g_faleweb_display_brush=CreateSolidBrush(g_high_contrast?background_color():
        (g_dark?RGB(19,48,86):RGB(247,250,254)));
    // Online / pending / offline status chips respect current light/dark theme.
    const COLORREF colors[3]={
        g_dark?RGB(71,31,39):RGB(251,232,235),
        g_dark?RGB(69,50,18):RGB(255,246,223),
        g_dark?RGB(23,62,45):RGB(225,247,233)
    };
    for(int i=0;i<3;++i){
        if(g_essential_status_brush[i])DeleteObject(g_essential_status_brush[i]);
        g_essential_status_brush[i]=CreateSolidBrush(colors[i]);
    }
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
    HFONT next[] = {create_font(15,FW_MEDIUM), create_font(27,FW_BOLD),
                    create_font(13,FW_NORMAL), create_font(21,FW_SEMIBOLD)};
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
    if(g_faleweb_digit_font)DeleteObject(g_faleweb_digit_font);
    if(g_faleweb_letters_font)DeleteObject(g_faleweb_letters_font);
    g_faleweb_digit_font=create_font(15,FW_SEMIBOLD);
    g_faleweb_letters_font=create_font(9,FW_NORMAL);
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
        if (page == 3) g_monitor_controls.push_back(child);
    }
    return child;
}
HWND make_label(HWND root, LPCWSTR text, int x, int y, int w, int h,
                int page = -1, HFONT font = nullptr, int automation_id = 0) {
    return make_control(root, L"STATIC", text, SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS,
                        x, y, w, h, automation_id, page, font);
}
HWND make_button(HWND root, LPCWSTR text, int id, int x, int y, int w, int h,
                 int page = -1, HFONT font = nullptr) {
    // BN_SETFOCUS drives viewport scrolling for keyboard navigation.
    // Compact Windows-style controls centered inside existing layout slots.
    const bool dial_key = id >= kDigitStart && id < kDigitStart + 12;
    const int inset_x = dial_key ? 0 : (w >= 80 ? 4 : 2);
    const int inset_y = dial_key ? 0 : (h >= 36 ? 3 : 1);
    return make_control(root, L"BUTTON", text, WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                        x + inset_x, y + inset_y, w - 2 * inset_x,
                        h - 2 * inset_y, id, page, font);
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
// Content beyond the available client height is reachable by scrollbar,
// mouse wheel and keyboard focus without cropping or changing SIP state.
// Explanatory feedback is anchored in a dedicated bottom bar.
int footer_height() { return axv::dpi_pixels(g_faleweb_mode?31:kFooterHeight,g_dpi); }
int content_viewport_height(HWND window) {
    RECT client{};
    GetClientRect(window,&client);
    const int height=static_cast<int>(client.bottom-client.top);
    return (std::max)(1,height-footer_height());
}
void position_footer(HWND window) {
    if(!g_status || !g_footer_panel || !window)return;
    RECT client{};
    GetClientRect(window,&client);
    const auto boxes=axv::footer_boxes(
        static_cast<int>(client.right-client.left),
        static_cast<int>(client.bottom-client.top),footer_height(),g_dpi);
    const auto& panel=boxes.panel;
    const auto& message=boxes.message;
    SetWindowPos(g_footer_panel,HWND_TOP,panel.x,panel.y,
                 panel.width,panel.height,SWP_NOACTIVATE);
    SetWindowPos(g_status,HWND_TOP,message.x,message.y,
                 message.width,message.height,SWP_NOACTIVATE);
}

void position_all_children() {
    // One deferred batch avoids dozens of synchronous redraws per scroll tick.
    // Never allow an invalid batch to leave controls stranded: fall back to
    // the existing individual positioning behavior.
    HDWP batch = BeginDeferWindowPos(static_cast<int>(g_layout.size()));
    if(batch) {
        for (const auto& child : g_layout) {
            if(child.handle==g_status)continue;
            auto r=child.logical;
            if (child.handle==g_number_label) r.y=g_compact?218:246;
            if (child.handle==g_number) r.y=g_compact?242:272;
            if (child.handle==g_status) r.y=g_compact?298:kFullFeedbackY;
            const auto box=axv::scaled_box(r,g_dpi);
            batch=DeferWindowPos(batch,child.handle,nullptr,
                box.x,box.y-g_scroll_offset,box.width,box.height,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW);
            if(!batch)break;
        }
        if(batch && EndDeferWindowPos(batch)){position_footer(GetParent(g_status));return;}
    }
    for(const auto& child:g_layout) {
        if(child.handle==g_status)continue;
        auto r=child.logical;
        if(child.handle==g_number_label)r.y=g_compact?218:246;
        if(child.handle==g_number)r.y=g_compact?242:272;
        if(child.handle==g_status)r.y=g_compact?298:kFullFeedbackY;
        position_logical(child.handle,r.x,r.y,r.width,r.height);
    }
    position_footer(GetParent(g_status));
}

int viewport_content_height() {
    return axv::dpi_pixels(g_compact ? kMiniScrollContentEnd
                                     : kFullScrollContentEnd, g_dpi);
}
void sync_viewport_scroll(HWND window) {
    if(g_faleweb_mode){g_scroll_max=0;g_scroll_offset=0;ShowScrollBar(window,SB_VERT,FALSE);return;}
    if (!g_controls_ready || g_scrollbar_updating) return;
    g_scrollbar_updating = true;
    RECT client{};
    GetClientRect(window, &client);
    const int content = viewport_content_height();
    const int available = content_viewport_height(window);
    const int previous = g_scroll_offset;
    g_scroll_max = axv::max_scroll_offset(content, available);
    g_scroll_offset = axv::clamp_scroll_offset(g_scroll_offset, content, available);
    SCROLLINFO bar{};
    bar.cbSize = sizeof(bar);
    bar.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    bar.nMin = 0;
    bar.nMax = content > 0 ? content - 1 : 0;
    bar.nPage = static_cast<UINT>(available > 0 ? available : 0);
    bar.nPos = g_scroll_offset;
    SetScrollInfo(window, SB_VERT, &bar, TRUE);
    ShowScrollBar(window, SB_VERT, g_scroll_max > 0);
    if (previous != g_scroll_offset) {
        position_all_children();
        InvalidateRect(window, nullptr, TRUE);
    }
    g_scrollbar_updating = false;
}
void set_viewport_scroll(HWND window, int desired) {
    if(g_faleweb_mode)return;
    RECT client{};
    GetClientRect(window, &client);
    const int target = axv::clamp_scroll_offset(desired, viewport_content_height(),
                                                 content_viewport_height(window));
    if (target == g_scroll_offset) return;
    g_scroll_offset = target;
    sync_viewport_scroll(window);
    position_all_children();
    InvalidateRect(window, nullptr, FALSE);
    RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_NOERASE);
}
void bring_focused_control_into_view(HWND window, HWND control) {
    if(g_faleweb_mode)return;
    if (!g_controls_ready || !control) return;
    for (const auto& child : g_layout) {
        if (child.handle != control) continue;
        int y = child.logical.y;
        if (control == g_number_label) y = g_compact ? 218 : 246;
        if (control == g_number) y = g_compact ? 242 : 272;
        if (control == g_status) y = g_compact ? 298 : kFullFeedbackY;
        RECT client{};
        GetClientRect(window, &client);
        const int top = axv::dpi_pixels(y,g_dpi);
        const int bottom = top + axv::dpi_pixels(child.logical.height,g_dpi);
        const int target = axv::scroll_for_visible_range(
            g_scroll_offset, viewport_content_height(), content_viewport_height(window),
            top, bottom, axv::dpi_pixels(8,g_dpi));
        set_viewport_scroll(window, target);
        break;
    }
}

void apply_faleweb_layout(HWND root);
void show_page(HWND root, int page) {
    if (g_page != page) {
        if (g_page == 2) { g_microphone_test.request_stop(); g_audio_change_policy.leave(); g_audio_worker.cancel(); g_audio_retry.reset(); g_audio_inventory_current=false; }
        if (g_page == 1) { g_settings_audio_worker.cancel(); g_settings_audio_retry.reset(); }
    }
    if (g_page != page && page == 2) g_audio_change_policy.enter(GetTickCount64());
    if (g_page != page) set_viewport_scroll(root, 0);
    g_page = page;
    for (auto child : g_dial_controls) {
        const bool visible = page == 0 &&
            (!g_compact || child == g_number || child == g_number_label);
        ShowWindow(child, visible ? SW_SHOW : SW_HIDE);
    }
    for (auto child : g_settings_controls) ShowWindow(child, page == 1 ? SW_SHOW : SW_HIDE);
    for (auto child : g_audio_controls)
        ShowWindow(child, page == 2 && child != g_headset_status ? SW_SHOW : SW_HIDE);
    for (auto child : g_monitor_controls) ShowWindow(child, page == 3 ? SW_SHOW : SW_HIDE);
    sync_viewport_scroll(root);
    InvalidateRect(root, nullptr, TRUE);
    HWND target = page == 0 ? g_number : (page == 1 ? g_server : (page == 2 ? g_input_audio_combo : g_monitor_events));
    if (target) SetFocus(target);
    if (page == 0) refresh_destination_feedback();
    else if (page == 1) refresh_account_feedback();
    else if (page == 3) {
        if (g_status) SetWindowTextW(g_status,L"Monitor: somente estados observados; nao captura pacotes SIP.");
    }
    else if(g_status && g_headset_status) {
        wchar_t explanation[512]{};
        GetWindowTextW(g_headset_status,explanation,512);
        SetWindowTextW(g_status,explanation);
    }
    if(g_faleweb_mode)apply_faleweb_layout(root);
}

// Mini mode keeps only the target and true SIP state, without fake call actions.
// FaleWEB-inspired compact dialer uses existing live SIP controls, not a
// duplicate engine. The existing large settings pages remain in memory only.
void position_essential(HWND child,int x,int y,int width,int height) {
    auto shrink=[](int value){return (value*kEssentialScalePercent+50)/100;};
    position_logical(child,shrink(x),shrink(y),shrink(width),shrink(height));
}
void apply_faleweb_layout(HWND root) {
    // Original FaleWEB smoke layout is retained unchanged for regression checks.
    // Normal desktop view follows the approved Essencial claro reference.
    if (g_faleweb_mode && !g_faleweb_smoke) {
        g_compact=false; g_scroll_offset=0; g_scroll_max=0;
        ShowScrollBar(root,SB_VERT,FALSE);
        // The compact Essencial canvas has an actual fourth-page monitor.
        // Toggle back to the dialer without resizing the main window.
        if (g_page == 3) {
            for (const auto& item : g_layout) {
                const int id=GetDlgCtrlID(item.handle);
                const bool shown=item.handle==g_brand_label ||
                    item.handle==g_runtime_banner ||
                    item.handle==g_monitor_events ||
                    id==kTheme || id==kShowSipMonitor;
                ShowWindow(item.handle,shown?SW_SHOW:SW_HIDE);
            }
            position_essential(g_brand_label,23,13,215,39);
            position_essential(GetDlgItem(root,kTheme),369,15,52,32);
            position_essential(GetDlgItem(root,kShowSipMonitor),253,15,108,32);
            position_essential(g_runtime_banner,23,68,397,31);
            position_essential(g_monitor_events,23,116,397,575);
            SetWindowTextW(GetDlgItem(root,kShowSipMonitor),L"TECLADO");
            ShowWindow(g_footer_panel,SW_HIDE);
            ShowWindow(g_status,SW_HIDE);
            InvalidateRect(root,nullptr,FALSE);
            return;
        }
        SetWindowTextW(GetDlgItem(root,kShowSipMonitor),L"MONITOR");
        for (const auto& item : g_layout) {
            const int id=GetDlgCtrlID(item.handle);
            const bool digit=id>=kDigitStart && id<kDigitStart+12;
            const bool shown=digit || item.handle==g_number ||
                item.handle==g_runtime_banner || item.handle==g_brand_label ||
                id==kTheme || id==kShowSipMonitor || id==kShowDial || id==kFalewebPrefsButton ||
                id==kContactsButton || id==kRecentDestinationsButton ||
                id==kCallButton || id==kMuteFuture || id==kHoldFuture ||
                id==kTransferFuture || id==kHangupFuture
#ifdef AXV_ENABLE_NATIVE_SIP
                || id==kAnswerNative
#endif
                ;
            ShowWindow(item.handle,shown?SW_SHOW:SW_HIDE);
        }
        // Logical coordinates reproduce the proportions of the first approved mockup.
        position_essential(g_brand_label,85,13,165,39);
        position_essential(GetDlgItem(root,kShowSipMonitor),253,15,108,32);
        position_essential(GetDlgItem(root,kTheme),369,15,52,32);
        position_essential(g_runtime_banner,26,68,391,31);
        position_essential(g_number,23,124,397,56);
        for(int i=0;i<12;++i)
            position_essential(GetDlgItem(root,kDigitStart+i),
                             23+(i%3)*132,192+(i/3)*68,129,64);
        // Preserve the approved 355x602 dialer. Only its call action row
        // changes in response to a verified native call identity.
        const auto view=g_essential_actions.view;
        const bool incoming=view==axv::EssentialCallView::Incoming;
        position_essential(GetDlgItem(root,kCallButton),23,477,397,54);
#ifdef AXV_ENABLE_NATIVE_SIP
        position_essential(GetDlgItem(root,kAnswerNative),23,477,194,54);
#endif
        position_essential(GetDlgItem(root,kHangupFuture),
                           incoming?225:23,477,incoming?195:397,54);
        position_essential(GetDlgItem(root,kMuteFuture),23,546,126,45);
        position_essential(GetDlgItem(root,kHoldFuture),158,546,126,45);
        position_essential(GetDlgItem(root,kTransferFuture),293,546,127,45);
        position_essential(GetDlgItem(root,kShowDial),23,622,94,67);
        position_essential(GetDlgItem(root,kContactsButton),125,622,94,67);
        position_essential(GetDlgItem(root,kRecentDestinationsButton),227,622,94,67);
        position_essential(g_faleweb_prefs_button,329,622,94,67);
        // Exactly one verified primary action group is shown at a time.
        // Visibility is presentation; command handlers still enforce native
        // session + displayed-call identity before any SIP operation.
        ShowWindow(GetDlgItem(root,kCallButton),
                   view==axv::EssentialCallView::Idle?SW_SHOW:SW_HIDE);
        ShowWindow(GetDlgItem(root,kHangupFuture),
                   g_essential_actions.hangup?SW_SHOW:SW_HIDE);
#ifdef AXV_ENABLE_NATIVE_SIP
        ShowWindow(GetDlgItem(root,kAnswerNative),
                   g_essential_actions.answer?SW_SHOW:SW_HIDE);
#endif
        const int secondary=g_essential_actions.secondary?SW_SHOW:SW_HIDE;
        ShowWindow(GetDlgItem(root,kMuteFuture),secondary);
        ShowWindow(GetDlgItem(root,kHoldFuture),secondary);
        ShowWindow(GetDlgItem(root,kTransferFuture),secondary);
        const HWND end_button=GetDlgItem(root,kHangupFuture);
        const wchar_t* end_label=incoming?L"Recusar":L"Encerrar";
        wchar_t previous_end[32]{};
        GetWindowTextW(end_button,previous_end,32);
        if(std::wcscmp(previous_end,end_label)!=0)
            SetWindowTextW(end_button,end_label);
        SetWindowTextW(GetDlgItem(root,kCallButton),L"Ligar");
        SetWindowTextW(GetDlgItem(root,kShowDial),L"Teclado");
        SetWindowTextW(GetDlgItem(root,kContactsButton),L"Contatos");
        SetWindowTextW(GetDlgItem(root,kRecentDestinationsButton),L"Histórico");
        SetWindowTextW(g_faleweb_prefs_button,L"Ajustes");
        SetWindowTextW(GetDlgItem(root,kHoldFuture),L"Espera");
        SetWindowTextW(GetDlgItem(root,kTransferFuture),L"Transferir");
        SetWindowTextW(GetDlgItem(root,kMuteFuture),L"Mudo");
        // The approved keypad has no legacy feedback footer overlaying navigation.
        ShowWindow(g_footer_panel,SW_HIDE);
        ShowWindow(g_status,SW_HIDE);
        InvalidateRect(root,nullptr,FALSE);
        return;
    }
    if(!g_faleweb_mode || !g_controls_ready && !g_runtime_banner)return;
    g_compact=false;g_scroll_offset=0;g_scroll_max=0;
    ShowScrollBar(root,SB_VERT,FALSE);
    // Hide all legacy page controls, then reveal only the selected controls.
    for(const auto& item:g_layout) {
        const int id=GetDlgCtrlID(item.handle);
        const bool digit=id>=kDigitStart && id<kDigitStart+12;
        const bool shown=digit || item.handle==g_number ||
             item.handle==g_runtime_banner || item.handle==g_brand_label ||
             id==kTheme ||
#ifdef AXV_ENABLE_NATIVE_SIP
             item.handle==g_call_duration_label ||
             item.handle==g_ringtone_status_label ||
#endif
             id==kCallButton || id==kHangupFuture || id==kMuteFuture ||
             id==kContactsButton || id==kRecentDestinationsButton ||
             id==kFalewebPrefsButton || id==kFalewebActionsButton
#ifdef AXV_ENABLE_NATIVE_SIP
             || id==kAnswerNative
#endif
             ;
        if(item.handle==g_status)continue; // Fixed status remains its own area.
        ShowWindow(item.handle,shown?SW_SHOW:SW_HIDE);
    }
    position_logical(g_brand_label,12,5,165,31);
    position_logical(GetDlgItem(root,kTheme),184,5,59,29);
    position_logical(g_runtime_banner,13,43,227,35);
#ifdef AXV_ENABLE_NATIVE_SIP
    position_logical(g_call_duration_label,17,82,210,21);
    position_logical(g_ringtone_status_label,130,101,109,19);
#endif
    position_logical(g_number,12,178,228,28);
    for(int i=0;i<12;++i){
        HWND digit=GetDlgItem(root,kDigitStart+i);
        position_logical(digit,14+(i%3)*77,211+(i/3)*35,72,33);
    }
    position_logical(GetDlgItem(root,kCallButton),10,351,113,28);
#ifdef AXV_ENABLE_NATIVE_SIP
    position_logical(GetDlgItem(root,kAnswerNative),128,351,112,28);
#endif
    position_logical(GetDlgItem(root,kHangupFuture),10,382,113,22);
    position_logical(GetDlgItem(root,kMuteFuture),128,382,112,22);
    position_logical(GetDlgItem(root,kContactsButton),10,150,112,23);
    position_logical(GetDlgItem(root,kRecentDestinationsButton),128,150,112,23);
    position_logical(g_faleweb_prefs_button,10,121,113,26);
    position_logical(g_faleweb_actions_button,128,121,112,26);
    position_footer(root);
    InvalidateRect(root,nullptr,FALSE);
}
void apply_compact_layout(HWND root) {
    if(g_faleweb_mode){
        // The 355x602 Essencial proportions are the preferred 96-DPI design.
        // On smaller Windows work areas a high scaling factor can make the
        // physical height exceed the monitor. Scale the *entire* Win32 canvas
        // down coherently (including its fonts) instead of clipping the
        // bottom navigation or introducing a forbidden vertical scrollbar.
        if(!g_faleweb_smoke) {
            MONITORINFO monitor{};
            monitor.cbSize=sizeof(monitor);
            const HMONITOR display=MonitorFromWindow(root,MONITOR_DEFAULTTONEAREST);
            if(display && GetMonitorInfoW(display,&monitor)) {
                const int work_height=monitor.rcWork.bottom-monitor.rcWork.top;
                constexpr int kWorkAreaMargin=16;
                if(work_height>kEssentialHeight+kWorkAreaMargin) {
                    const unsigned maximum=static_cast<unsigned>(
                        (static_cast<std::int64_t>(work_height-kWorkAreaMargin)*96)
                        /kEssentialHeight);
                    const unsigned fitted=(std::min)(g_dpi,
                        axv::normalized_dpi(maximum));
                    if(fitted!=g_dpi){
                        g_dpi=fitted;
                        rebuild_fonts();
                    }
                }
            }
        }
        SetWindowPos(root,nullptr,0,0,axv::dpi_pixels(g_faleweb_smoke?kFalewebWidth:kEssentialWidth,g_dpi),
            axv::dpi_pixels(g_faleweb_smoke?kFalewebHeight:kEssentialHeight,g_dpi),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        apply_faleweb_layout(root);return;
    }
    if (g_compact && g_page != 0) show_page(root, 0);
    position_logical(g_number_label,39,g_compact?218:246,320,20);
    position_logical(g_number,39,g_compact?242:272,350,43);
    position_footer(root);
    SetWindowTextW(GetDlgItem(root, kCompact), g_compact ? L"MAX" : L"MINI");
    const int desired_height = axv::dpi_pixels(g_compact?kCompactHeight:kFullHeight,g_dpi);
    int fitted_height = desired_height;
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    const HMONITOR display = MonitorFromWindow(root, MONITOR_DEFAULTTONEAREST);
    if (display && GetMonitorInfoW(display, &monitor)) {
        const int work_height = monitor.rcWork.bottom-monitor.rcWork.top;
        if (work_height > 0) fitted_height = (std::min)(desired_height, work_height);
    }
    const int width = axv::dpi_pixels(kWindowWidth,g_dpi) +
                      GetSystemMetricsForDpi(SM_CXVSCROLL,g_dpi);
    SetWindowPos(root, nullptr, 0, 0, width, fitted_height,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    show_page(root, g_page);
    sync_viewport_scroll(root);
    position_footer(root);
}
void draw_card(HDC dc, RECT rect, COLORREF color, int radius) {
    HBRUSH fill = CreateSolidBrush(color);
    HPEN border = CreatePen(PS_SOLID, 1, line_color());
    HGDIOBJ old_brush = SelectObject(dc, fill);
    HGDIOBJ old_pen = SelectObject(dc, border);
    const int diameter=axv::rounded_rect_diameter(
        static_cast<int>(rect.right-rect.left),
        static_cast<int>(rect.bottom-rect.top),
        axv::dpi_pixels(radius,g_dpi));
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom,
              diameter,diameter);
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
                              (id == kShowAudio && g_page == 2) ||
                               (id == kShowSipMonitor && g_page == 3);
    // Call controls: green for initiating, red for ending/stopping.
    // High contrast and disabled states keep the Windows/theme palette.
    const bool positive = id == kCallButton || id == kAnswerNativeColorId ||
                          id == kStartBaresipButton;
    const bool negative = id == kHangupFuture || id == kStopBaresipButton;
    const bool semantic = (positive || negative) && !g_high_contrast;
    COLORREF color = ui_color(axv::UiRole::Button);
    if (selected_tab || id == kCallButton) color = accent_color();
    if (id >= kDigitStart && id < kDigitStart + 12) color = field_color();
    if (semantic) color = positive ? RGB(20,126,72) : RGB(185,40,52);
    if (pressed) color = semantic
        ? (positive ? RGB(12,96,53) : RGB(145,27,41))
        : ui_color(axv::UiRole::Pressed);
    if (disabled) color = ui_color(axv::UiRole::DisabledButton);

    RECT rect = item.rcItem;
    HBRUSH fill = CreateSolidBrush(color);
    HPEN outline = CreatePen(PS_SOLID, 1, selected_tab ? accent_color() : line_color());
    HGDIOBJ brush_old = SelectObject(item.hDC, fill);
    HGDIOBJ pen_old = SelectObject(item.hDC, outline);
    const int offset=axv::dpi_pixels(1,g_dpi);
    // More delicate and rounded controls. Clamp to the actual owner-drawn
    // rectangle so small buttons and 125/150% DPI never paint outside bounds.
    const int curve=axv::compact_button_diameter(
        static_cast<int>(rect.right-rect.left),
        static_cast<int>(rect.bottom-rect.top),
        g_dpi,g_faleweb_mode,offset);
    RoundRect(item.hDC,rect.left+offset,rect.top+offset,
              rect.right-offset,rect.bottom-offset,curve,curve);
    SelectObject(item.hDC, pen_old);
    SelectObject(item.hDC, brush_old);
    DeleteObject(outline);
    DeleteObject(fill);

    const COLORREF foreground = disabled ? ui_color(axv::UiRole::DisabledText)
        : (semantic ? RGB(255,255,255)
           : (selected_tab || id == kCallButton
              ? ui_color(axv::UiRole::SelectedText) : ui_color(axv::UiRole::ButtonText)));
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, foreground);
    wchar_t title[96] = {};
    GetWindowTextW(item.hwndItem, title, 95);

    if (id >= kDigitStart && id < kDigitStart + 12) {
        const auto index = static_cast<size_t>(id - kDigitStart);
        const auto digit = axv::dialpad_keys()[index];
        const wchar_t primary[2] = {digit, 0};
        // Measure actual glyph heights rather than fixed Y offsets. A pressed
        // button must never let the digit and the subtitle paint over each other.
        const wchar_t* letters[] = {
            L"", L"ABC", L"DEF", L"GHI", L"JKL", L"MNO",
            L"PQRS", L"TUV", L"WXYZ", L"", L"", L""
        };
        const int padding = axv::dpi_pixels(3,g_dpi);
        const int gap = axv::dpi_pixels(2,g_dpi);
        HFONT primary_font=g_faleweb_mode && g_faleweb_digit_font?g_faleweb_digit_font:g_font_digit;
        HFONT secondary_font=g_faleweb_mode && g_faleweb_letters_font?g_faleweb_letters_font:g_font_small;
        HGDIOBJ old = SelectObject(item.hDC,primary_font);
        TEXTMETRICW digit_metrics{};
        GetTextMetricsW(item.hDC,&digit_metrics);
        SelectObject(item.hDC,secondary_font);
        TEXTMETRICW letter_metrics{};
        GetTextMetricsW(item.hDC,&letter_metrics);
        const int inner_height = (rect.bottom-rect.top)-2*padding;
        const bool draw_letters = letters[index][0] &&
            inner_height >= digit_metrics.tmHeight + gap + letter_metrics.tmHeight;
        const int group_height = digit_metrics.tmHeight +
            (draw_letters ? gap + letter_metrics.tmHeight : 0);
        const int top = rect.top + padding + ((std::max)(0,inner_height-group_height))/2;
        RECT number{rect.left+padding,top,rect.right-padding,top+digit_metrics.tmHeight};
        SelectObject(item.hDC,primary_font);
        SetTextColor(item.hDC,foreground);
        DrawTextW(item.hDC,primary,-1,&number,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (draw_letters) {
            RECT subtitle{rect.left+padding,number.bottom+gap,
                          rect.right-padding,number.bottom+gap+letter_metrics.tmHeight};
            SelectObject(item.hDC,secondary_font);
            SetTextColor(item.hDC,disabled ? ui_color(axv::UiRole::DisabledText) : muted_color());
            DrawTextW(item.hDC,letters[index],-1,&subtitle,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        SelectObject(item.hDC, old);
    } else {
        HGDIOBJ old_font = SelectObject(item.hDC,
            id==kTheme && g_faleweb_mode ? g_font_small : g_font_regular);
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

bool can_preview_selected_ringtone(){
    if(g_smoke_mode || g_tone_preview.state()==axv::TonePreviewState::Playing ||
        g_microphone_test.state()==axv::MicrophoneTestState::Capturing)return false;
    if(g_ringtone_preset.load()==5){
        std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);
        if(g_ringtone_custom_mp3.empty())return false;
#ifdef AXV_ENABLE_NATIVE_SIP
        return !g_ring_player.busy() && !axv::verified_incoming_call(native_engine().observation());
#else
        return true;
#endif
    }
    if(!g_audio_inventory_current)return false;
#ifdef AXV_ENABLE_NATIVE_SIP
    if(g_ring_player.busy() || axv::verified_incoming_call(native_engine().observation()))return false;
    return axv::fixed_native_ringtone_selection(g_audio_outputs,g_selected_ringtone_id).available();
#else
    return !g_selected_ringtone_id.empty() && std::any_of(g_audio_outputs.begin(),g_audio_outputs.end(),[](const auto& device){return device.id==g_selected_ringtone_id;});
#endif
}
// The selected conversation endpoint can be tested without starting SIP/RTP.
// Refuse to overlap a call or the incoming alert on a shared output.
bool can_preview_selected_conversation() {
    if (g_smoke_mode || !g_audio_inventory_current ||
        g_tone_preview.state() == axv::TonePreviewState::Playing ||
        g_microphone_test.state() == axv::MicrophoneTestState::Capturing) return false;
#ifdef AXV_ENABLE_NATIVE_SIP
    if (g_ring_player.busy() || native_engine().observation().call_id != 0)
        return false;
    return axv::audio_output_preview_endpoint(
        g_audio_outputs,g_selected_output_id,true,
        axv::AudioOutputPreviewRole::Conversation).has_value();
#else
    return axv::audio_output_preview_endpoint(
        g_audio_outputs,g_selected_output_id,false,
        axv::AudioOutputPreviewRole::Conversation).has_value();
#endif
}
bool can_test_selected_microphone() {
    if (g_smoke_mode || !g_audio_inventory_current ||
        g_microphone_test.state()==axv::MicrophoneTestState::Capturing ||
        g_tone_preview.state()==axv::TonePreviewState::Playing) return false;
#ifdef AXV_ENABLE_NATIVE_SIP
    if (g_ring_player.busy() ||
        native_engine().observation().call_id != 0 ||
        axv::verified_incoming_call(native_engine().observation())) return false;
    constexpr bool follow_default = true;
#else
    constexpr bool follow_default = false;
#endif
    return axv::microphone_test_endpoint(
        g_audio_inputs,g_selected_input_id,follow_default).has_value();
}
void refresh_audio_test_actions(HWND window) {
    EnableWindow(GetDlgItem(window,kTestRingtoneButton),
                 can_preview_selected_ringtone());
    EnableWindow(GetDlgItem(window,kTestConversationButton),
                 can_preview_selected_conversation());
    EnableWindow(GetDlgItem(window,kTestMicrophoneButton),
                 can_test_selected_microphone());
}
#ifdef AXV_ENABLE_NATIVE_SIP
void acknowledge_incoming_alert(){g_ring_policy.silence(g_displayed_sip);g_ring_player.stop();}
void synchronize_incoming_alert(const axv::SipRuntimeObservation& observed){
    g_displayed_sip=observed;
    // Alert only once per verified incoming native call. Never infer an INVITE
    // from registration or engine process state. Do not steal window focus.
    if(!g_smoke_mode && axv::verified_incoming_call(observed) &&
       (observed.session!=g_last_incoming_visual_session ||
        observed.call_id!=g_last_incoming_visual_call)){
        g_last_incoming_visual_session=observed.session;
        g_last_incoming_visual_call=observed.call_id;
        HWND owner=GetParent(g_runtime_banner);
        if(owner){
            if(g_tray_visible){
                auto tray=create_tray_data(owner);
                tray.uFlags=NIF_INFO;
                wcscpy_s(tray.szInfoTitle,L"AXE Voice - chamada recebida");
                wcscpy_s(tray.szInfo,L"Uma chamada esta chegando. Abra o AXE Voice para atender.");
                tray.dwInfoFlags=NIIF_INFO|NIIF_NOSOUND;
                (void)Shell_NotifyIconW(NIM_MODIFY,&tray);
            }else{
                FLASHWINFO flash{sizeof(flash),owner,FLASHW_TRAY|FLASHW_TIMERNOFG,6,0};
                (void)FlashWindowEx(&flash);
            }
        }
    }
    const auto request=g_smoke_mode?axv::IncomingRingtoneRequest{}:g_ring_policy.request(observed,g_selected_ringtone_id,g_tone_preview.state()==axv::TonePreviewState::Playing,g_incoming_ringtone_muted);
    if(axv::verified_incoming_call(observed)){g_tone_preview.request_stop();g_microphone_test.request_stop();}
    const bool accepted=g_ring_player.synchronize(request);
    const auto state=g_ring_player.snapshot();
    const bool matching=state.request.session==request.session && state.request.call_id==request.call_id;
    const bool error=request.active() && (!accepted || (matching &&
        (state.result==axv::TonePreviewState::MissingDevice || state.result==axv::TonePreviewState::UnsupportedFormat || state.result==axv::TonePreviewState::Failed)));
    if(error){
        const wchar_t* message=L"Toque indisponivel na saida escolhida. Use o aviso visual para atender.";
        wchar_t previous[180]{};GetWindowTextW(g_status,previous,180);
        if(std::wstring_view(previous)!=message)SetWindowTextW(g_status,message);
        g_ring_error_visible=true;
    }else if(g_ring_error_visible){g_ring_error_visible=false;refresh_destination_feedback();}
    refresh_audio_test_actions(GetParent(g_runtime_banner));
}
#endif
// Native capture/playback selections are applied at registration; ringtone is an independent incoming-call alert.
axv::AudioSelectionRefresh fill_audio_combo(
    HWND combo, const std::vector<axv::AudioEndpoint>& devices,
    std::wstring& selected_id, bool ringtone = false, bool fixed_route = false, bool native_route = false) {
    const auto selection = (fixed_route || native_route) ? (ringtone?axv::fixed_native_ringtone_selection(devices,selected_id):axv::fixed_native_audio_selection(devices,selected_id)) : ringtone
        ? axv::refresh_ringtone_selection(devices, selected_id)
        : axv::refresh_audio_selection(devices, selected_id);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    if(native_route)SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(ringtone?L"[WINDOWS] Padrao multimedia":L"[WINDOWS] Padrao de comunicacao"));
    for (const auto& device : devices) {
        const std::wstring prefix = ringtone
            ? (device.default_multimedia ? L"[PADRAO TOQUE] " : L"")
            : (device.default_communications ? L"[PADRAO] " : L"");
        const std::wstring title = prefix + device.name;
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(title.c_str()));
    }
    EnableWindow(combo, !fixed_route && (native_route || selection.index >= 0));
    const int row=native_route?axv::native_audio_combo_index(devices,selected_id):selection.index;
    if (row >= 0) {
        SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(row), 0);
    }
    selected_id = selection.selected_id;
    return selection;
}

void apply_audio_inventory(const axv::AudioInventory& inventory) {
    bool fixed_routes=false,native_routes=false;
#ifdef AXV_ENABLE_NATIVE_SIP
    native_routes=true;
    fixed_routes=native_engine().observation().process!=axv::RuntimeProcess::Stopped;
    if(fixed_routes){
        const auto route=native_engine().selected_audio_route();
        g_selected_input_id=route.capture_id;g_selected_output_id=route.playback_id;
    }
#endif
    g_audio_inventory_current = inventory.error == axv::AudioInventoryError::None;
    const auto selection=axv::reconcile_audio_panel(
        {g_selected_input_id,g_selected_output_id,g_selected_ringtone_id},inventory,native_routes);
    g_selected_input_id=selection.input_id;
    g_selected_output_id=selection.output_id;
    g_selected_ringtone_id=selection.ringtone_id;
    if (!g_audio_inventory_current) {
        EnableWindow(g_input_audio_combo,FALSE);
        EnableWindow(g_output_audio_combo,FALSE);
        EnableWindow(g_ringtone_audio_combo,FALSE);
        EnableWindow(GetDlgItem(GetParent(g_ringtone_audio_combo), kTestRingtoneButton), FALSE);
        EnableWindow(GetDlgItem(GetParent(g_ringtone_audio_combo), kTestConversationButton), FALSE);
        EnableWindow(GetDlgItem(GetParent(g_ringtone_audio_combo), kTestMicrophoneButton), FALSE);
        SetWindowTextW(g_headset_status, L"Falha ao consultar os dispositivos do Windows.");
        return;
    }
    g_audio_inputs = inventory.input_devices;
    g_audio_outputs = inventory.output_devices;
    const auto microphone = fill_audio_combo(
        g_input_audio_combo, g_audio_inputs, g_selected_input_id,false,fixed_routes,native_routes);
    const auto speaker = fill_audio_combo(
        g_output_audio_combo, g_audio_outputs, g_selected_output_id,false,fixed_routes,native_routes);
    const auto ringtone = fill_audio_combo(
        g_ringtone_audio_combo, g_audio_outputs, g_selected_ringtone_id, true,false,native_routes);
    refresh_audio_test_actions(GetParent(g_ringtone_audio_combo));
    SetWindowTextW(g_headset_status,
        axv::audio_routing_preview_feedback({microphone, speaker, ringtone}));
#ifdef AXV_ENABLE_NATIVE_SIP
    const auto route_status=axv::native_audio_route_status({g_selected_input_id,g_selected_output_id},inventory);
    SetWindowTextW(g_headset_status,route_status==axv::NativeAudioRouteStatus::Ready
        ? (fixed_routes?L"Rota da sessao fixada. Pare o motor para trocar; teste com headset pendente.":L"Microfone e saida serao aplicados no proximo registro.")
        :axv::native_audio_route_feedback(route_status));
#endif
}

void request_audio_panel() {
    if (g_smoke_mode) return;
    g_audio_inventory_current=false;
    g_audio_worker.request();
    g_audio_change_policy.refreshed(GetTickCount64());
    EnableWindow(g_input_audio_combo,FALSE);
    EnableWindow(g_output_audio_combo,FALSE);
    EnableWindow(g_ringtone_audio_combo,FALSE);
    EnableWindow(GetDlgItem(GetParent(g_ringtone_audio_combo),kTestRingtoneButton),FALSE);
    EnableWindow(GetDlgItem(GetParent(g_ringtone_audio_combo),kTestConversationButton),FALSE);
    EnableWindow(GetDlgItem(GetParent(g_ringtone_audio_combo),kTestMicrophoneButton),FALSE);
    SetWindowTextW(g_headset_status,L"Consultando dispositivos de audio...");
}
bool audio_dropdown_open() {
    return SendMessageW(g_input_audio_combo,CB_GETDROPPEDSTATE,0,0) ||
        SendMessageW(g_output_audio_combo,CB_GETDROPPEDSTATE,0,0) ||
        SendMessageW(g_ringtone_audio_combo,CB_GETDROPPEDSTATE,0,0);
}
// GUI-thread notification handling invalidates the current generation immediately.
// Device identities and open dropdown contents remain untouched until refresh.
void audio_devices_changed(ULONGLONG now) {
    if (g_smoke_mode || g_compact || (g_page != 2 && !g_faleweb_mode)) return;
    g_audio_worker.cancel();
    g_audio_retry.reset();
    g_audio_inventory_current=false;
    g_audio_change_policy.changed(now);
}
void poll_audio_notifications() {
    if (g_smoke_mode) return;
    const auto now=GetTickCount64();
    if (g_audio_endpoint_watcher.take_changed()) audio_devices_changed(now);
    if (!g_compact && (g_page==2 || g_faleweb_mode) &&
        axv::audio_device_refresh_due(g_audio_change_policy,now,audio_dropdown_open(),g_audio_worker.snapshot())) request_audio_panel();
}
void poll_audio_inventory() {
    if (g_smoke_mode || g_compact) return;
    const auto now=GetTickCount64();
    if((g_page==2 || g_faleweb_mode) && !audio_dropdown_open()) {
        if(g_audio_worker.snapshot().pending && g_audio_retry.ready(now)) {
            if(g_audio_worker.start_pending()) g_audio_retry.started();
            else if(g_audio_worker.snapshot().launch_failed) g_audio_retry.failed(now);
        }
        axv::AudioInventory inventory;
        if(g_audio_worker.take(inventory)) {
            g_audio_change_policy.completed(now);
            apply_audio_inventory(inventory);
        }
    }
    if(g_page==1) {
        if(g_settings_audio_worker.snapshot().pending && g_settings_audio_retry.ready(now)) {
            if(g_settings_audio_worker.start_pending()) g_settings_audio_retry.started();
            else if(g_settings_audio_worker.snapshot().launch_failed) g_settings_audio_retry.failed(now);
        }
        axv::AudioInventory inventory;
        if(g_settings_audio_worker.take(inventory)) {
            const auto summary=axv::audio_inventory_summary(inventory);
            SetWindowTextW(g_audio_status,summary.c_str());
        }
    }
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
axv::SipRuntimeObservation current_sip_observation(axv::ProcessStatus state) {
#ifdef AXV_ENABLE_NATIVE_SIP
    (void)state;
    return native_voice_service().snapshot(false).observation;
#else
    return g_sip_session_guard.observe(observed_process(state,g_shutdown.pending()),
        axv::SipEvidence::None,g_native_event_mailbox,g_sip_projection);
#endif
}
#ifdef AXV_ENABLE_NATIVE_SIP
void clear_recent_destinations(HWND window) {
    g_recent_destinations.clear();
    EnableWindow(GetDlgItem(window,kRecentDestinationsButton),FALSE);
    SetWindowTextW(g_status,L"Lista de destinos recentes limpa.");
}
bool choose_recent_destination(HWND window,std::uint64_t token) {
    const auto current=current_sip_observation(g_baresip_process.poll());
    if(!axv::fill_recent_destination(g_recent_destinations,token,current,g_number)) {
        SetWindowTextW(g_status,L"Destino recente indisponivel; a sessao pode ter mudado.");return false;
    }
    show_page(window,0);
    SendMessageW(g_number,EM_SETSEL,0,-1);
    SetWindowTextW(g_status,L"Destino recente preenchido. Revise e clique CHAMAR.");
    return true;
}
void show_recent_destinations(HWND window) {
    g_recent_destinations.synchronize(current_sip_observation(g_baresip_process.poll()));
    if(!g_recent_destinations.size()) {
        SetWindowTextW(g_status,L"Sem discagens solicitadas nesta sessao. Recentes ficam somente na memoria.");return;
    }
    struct Menu {HMENU value=CreatePopupMenu();~Menu(){if(value)DestroyMenu(value);}} menu;
    if(!menu.value){SetWindowTextW(g_status,L"Nao foi possivel abrir os destinos recentes.");return;}
    try {
        std::array<std::uint64_t,axv::RecentDestinations::capacity> tokens{};
        if(!AppendMenuW(menu.value,MF_STRING|MF_GRAYED,0,L"Discagens solicitadas nesta sessao"))return;
        const auto count=g_recent_destinations.size();
        for(std::size_t i=0;i<count;++i) {
            const auto* entry=g_recent_destinations.at(i);
            tokens[i]=entry->token;
            const auto label=std::to_wstring(i+1)+L". "+entry->destination;
            if(!AppendMenuW(menu.value,MF_STRING,static_cast<UINT_PTR>(i+1),label.c_str()))return;
        }
        if(!AppendMenuW(menu.value,MF_SEPARATOR,0,nullptr) ||
           !AppendMenuW(menu.value,MF_STRING,100,L"LIMPAR RECENTES") ||
           !AppendMenuW(menu.value,MF_STRING|MF_GRAYED,0,L"Escolha preenche; use CHAMAR para discar."))return;
        RECT anchor{};GetWindowRect(g_number,&anchor);
        const auto chosen=TrackPopupMenu(menu.value,TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,
                                         anchor.left,anchor.bottom,0,window,nullptr);
        if(chosen==100)clear_recent_destinations(window);
        else if(chosen>=1 && static_cast<std::size_t>(chosen)<=count)
            (void)choose_recent_destination(window,tokens[static_cast<std::size_t>(chosen)-1]);
    }catch(...){SetWindowTextW(g_status,L"Nao foi possivel preparar a lista de destinos recentes.");}
}
#endif

// Monitor records only trusted state observations, not SIP wire packets.
// Avoid usernames, phone numbers, passwords, URIs and raw packet payloads.
void observe_monitor_state(const axv::SipRuntimeObservation& o) {
    if (!g_monitor_events) return;
    if (g_monitor_has_last &&
        g_monitor_last.process == o.process &&
        g_monitor_last.sip == o.sip &&
        g_monitor_last.evidence == o.evidence &&
        g_monitor_last.session == o.session &&
        g_monitor_last.call_id == o.call_id) return;
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t timestamp[32]{};
    swprintf_s(timestamp,L"%02u:%02u:%02u",now.wHour,now.wMinute,now.wSecond);
    const wchar_t* label = L"Motor sem evidencia de registro";
    if (o.process == axv::RuntimeProcess::Stopped) label=L"Motor parado";
    else if (o.process == axv::RuntimeProcess::Stopping) label=L"Motor encerrando";
    else if (o.process == axv::RuntimeProcess::Failed) label=L"Falha do motor";
    else if (axv::sip_proof_is_current(o,o.session)) {
        switch(o.sip){
        case axv::SipStatus::Registered: label=L"Registro SIP confirmado";break;
        case axv::SipStatus::Registering: label=L"Registro em andamento";break;
        case axv::SipStatus::Ringing: label=L"Chamada entrante sinalizada";break;
        case axv::SipStatus::Calling: label=L"Chamada originada sinalizada";break;
        case axv::SipStatus::InCall: label=L"Chamada estabelecida";break;
        case axv::SipStatus::Failed: label=L"Falha no registro";break;
        case axv::SipStatus::Disconnected: label=L"Transporte desconectado";break;
        default: label=L"SIP indisponivel";break;
        }
    }
    std::wstring entry=std::wstring(timestamp)+L"  "+label+L"\r\n";
    g_monitor_text += entry;
    // Bounded, ephemeral ring of 80 transitions, no disk persistence.
    int count=0;
    for(wchar_t ch:g_monitor_text) if(ch==L'\n') ++count;
    while(count>80){
        auto pos=g_monitor_text.find(L'\n');
        if(pos==std::wstring::npos)break;
        g_monitor_text.erase(0,pos+1);
        --count;
    }
    g_monitor_last=o;
    g_monitor_has_last=true;
    SetWindowTextW(g_monitor_events,g_monitor_text.c_str());
    SendMessageW(g_monitor_events,EM_SETSEL,g_monitor_text.size(),g_monitor_text.size());
    SendMessageW(g_monitor_events,EM_SCROLLCARET,0,0);
}

void refresh_runtime_banner(axv::ProcessStatus state) {
    if (!g_runtime_banner) return;
    // Thread-safe receive boundary is active, but not connected to Baresip C API.
    // Bind native event lifetime to actual process lifetime, not to a timer tick.
    const auto observed=current_sip_observation(state);
    observe_monitor_state(observed);
#ifdef AXV_ENABLE_NATIVE_SIP
    g_cti_server.set_ready(
        observed.call_id == 0 &&
        axv::authorize_call_command(observed,observed.session,g_sip_selection,
            axv::CallAction::Dial,0,L"1000").allowed());
    g_cti_server.observe(observed);
    if(observed.call_id!=0)g_microphone_test.request_stop();
    g_recent_destinations.synchronize(observed);
    EnableWindow(GetDlgItem(GetParent(g_runtime_banner),kRecentDestinationsButton),g_recent_destinations.size()!=0);
    synchronize_incoming_alert(observed);
    const auto duration=g_call_duration.observe(observed,GetTickCount64());
    std::wstring duration_text=duration.active
        ? L"DURACAO " + axv::format_call_duration(duration.elapsed_ms)
        : L"DURACAO --:--:--";
    // Keep mute status separate from call duration to avoid text clipping.
    const wchar_t* ringtone_text=g_incoming_ringtone_muted?L"TOQUE MUDO":L"";
    if(g_ringtone_status_label){
        wchar_t previous_ringtone[32]{};
        GetWindowTextW(g_ringtone_status_label,previous_ringtone,32);
        if(std::wstring_view(previous_ringtone)!=ringtone_text)
            SetWindowTextW(g_ringtone_status_label,ringtone_text);
    }
    SetWindowTextW(GetDlgItem(GetParent(g_runtime_banner),kIncomingRingtoneMuteButton),
                   g_incoming_ringtone_muted?L"TOQUE MUDO":L"TOQUE NORMAL");
    wchar_t previous_duration[80]{};
    GetWindowTextW(g_call_duration_label,previous_duration,80);
    if (std::wstring_view(previous_duration)!=duration_text)
        SetWindowTextW(g_call_duration_label,duration_text.c_str());
#endif
    const auto current_session = observed.session;
    const auto signal=axv::essential_connection_signal(observed,current_session);
    if(g_essential_signal!=signal){
        g_essential_signal=signal;
        InvalidateRect(g_runtime_banner,nullptr,TRUE);
    }
    // Process state never implies SIP registration proof.
    // Without a native verified producer the UI cannot claim SIP registration.
    const wchar_t* banner = axv::runtime_status_banner(observed, current_session);
    if(g_faleweb_mode) {
        if(observed.process==axv::RuntimeProcess::Running &&
           observed.sip==axv::SipStatus::Registering)
            banner=L"REGISTRANDO SIP...";
        else if(!axv::sip_proof_is_current(observed,current_session))
            banner=L"SIP NAO REGISTRADO";
        else switch(observed.sip){
        case axv::SipStatus::Registered:banner=L"SIP REGISTRADO";break;
        case axv::SipStatus::Ringing:banner=L"CHAMADA RECEBIDA";break;
        case axv::SipStatus::Calling:banner=L"CHAMANDO...";break;
        case axv::SipStatus::InCall:banner=L"EM CONVERSA";break;
        default:banner=L"SIP SEM CONEXAO";break;
        }
    }
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
    // UI never equates a process-up event with a verified SIP call.
    const auto actions=axv::essential_call_actions(observed,current_session,dial);
    const bool changed=actions.view!=g_essential_actions.view;
    g_essential_actions=actions;
    if(changed && g_faleweb_mode && !g_faleweb_smoke && g_page==0)
        apply_faleweb_layout(GetParent(g_runtime_banner));
#ifdef AXV_ENABLE_NATIVE_SIP
    const wchar_t* dial_label=g_faleweb_mode?L"Ligar":L"CHAMAR";
    wchar_t old_dial_label[40]{};
    GetWindowTextW(button,old_dial_label,40);
    if(std::wcscmp(old_dial_label,dial_label)!=0)
        SetWindowTextW(button,dial_label);
    EnableWindow(GetDlgItem(GetParent(g_runtime_banner),kAnswerNative),actions.answer);
    EnableWindow(GetDlgItem(GetParent(g_runtime_banner),kHangupFuture),actions.hangup);
    const auto media=native_engine().media_state();
    const bool controls=axv::native_media_controls_current(media,observed);
    g_displayed_media=controls?media:axv::NativeCallMediaState{};
    g_dtmf_mode.synchronize(media,observed);
    const HWND dtmf=GetDlgItem(GetParent(g_runtime_banner),kDtmfButton);
    EnableWindow(dtmf,axv::native_dtmf_controls_current(media,observed));
    SetWindowTextW(dtmf,g_dtmf_mode.armed()?L"DTMF ON":L"DTMF OFF");
    const HWND mute=GetDlgItem(GetParent(g_runtime_banner),kMuteFuture);
    const HWND hold=GetDlgItem(GetParent(g_runtime_banner),kHoldFuture);
    EnableWindow(mute,controls);EnableWindow(hold,controls);
    const HWND transfer=GetDlgItem(GetParent(g_runtime_banner),kTransferFuture);
    EnableWindow(transfer,controls && media.transfer==axv::NativeTransferState::None && axv::evaluate_destination(destination).valid());
    SetWindowTextW(transfer,media.transfer==axv::NativeTransferState::Submitted?L"TRANSF. PEDIDA":media.transfer==axv::NativeTransferState::Failed?L"TRANSF. FALHOU":L"TRANSFERIR");
    SetWindowTextW(mute,controls && media.muted?L"MIC MUDO":L"MUDO");
    SetWindowTextW(hold,controls && media.local_hold?L"RETOMAR":L"ESPERA");
    const bool routes_locked=observed.process!=axv::RuntimeProcess::Stopped;
    if (!SendMessageW(g_input_audio_combo,CB_GETDROPPEDSTATE,0,0))
        EnableWindow(g_input_audio_combo,!routes_locked && g_audio_inventory_current && SendMessageW(g_input_audio_combo,CB_GETCOUNT,0,0)>0);
    if (!SendMessageW(g_output_audio_combo,CB_GETDROPPEDSTATE,0,0))
        EnableWindow(g_output_audio_combo,!routes_locked && g_audio_inventory_current && SendMessageW(g_output_audio_combo,CB_GETCOUNT,0,0)>0);
    static std::wstring last_native_diagnostic;
    const auto diagnostic=native_engine().diagnostic();
    if(diagnostic!=last_native_diagnostic){SetWindowTextW(g_process_status,diagnostic.c_str());last_native_diagnostic=diagnostic;}

#endif
}
axv::CallReadiness collect_call_readiness() {
    wchar_t number[129]{}, server[129]{}, user[129]{}, port[129]{};
    GetWindowTextW(g_number, number, 129);
    GetWindowTextW(g_server, server, 129);
    GetWindowTextW(g_user, user, 129);
    GetWindowTextW(g_port, port, 129);
    const auto snapshot = current_sip_observation(g_baresip_process.poll());
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
    result.dial_control_available = axv::authorize_dial(snapshot, snapshot.session, g_sip_selection, number).allowed();
    return result;
}
axv::NativeSecurityStatus collect_security_status(){
#ifdef AXV_ENABLE_NATIVE_SIP
    return native_engine().security_status();
#else
    return {};
#endif
}
std::wstring make_readiness_report(){return axv::format_call_readiness(collect_call_readiness())+L"\n\n"+axv::format_native_security_status(collect_security_status());}
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

bool try_dial_from_ui(HWND window,const std::wstring& typed_destination) {
    // Same fresh authorization for manual CHAMAR and authenticated local CTI.
    // A successful start_call() means "submitted", never "connected".
    const auto observed=current_sip_observation(g_baresip_process.poll());
    const auto permission=axv::authorize_dial(
        observed,observed.session,g_sip_selection,typed_destination);
    if(!permission.allowed()) {
        SetWindowTextW(g_status,axv::dial_denial_text(permission.reason));
        return false;
    }
    g_microphone_test.request_stop();
#ifdef AXV_ENABLE_NATIVE_SIP
    const bool submitted=native_voice_service().dial(
        observed,permission.normalized_destination);
#else
    const bool submitted=g_sip_selection.engine->start_call(
        permission.normalized_destination);
#endif
    if(!submitted) {
        SetWindowTextW(g_status,g_sip_selection.engine->diagnostic().c_str());
        return false;
    }
#ifdef AXV_ENABLE_NATIVE_SIP
    (void)g_recent_destinations.remember(observed,typed_destination,true);
    EnableWindow(GetDlgItem(window,kRecentDestinationsButton),g_recent_destinations.size()!=0);
#endif
    return true;
}
#ifdef AXV_ENABLE_NATIVE_SIP
void dispatch_pending_cti(HWND window) {
    if (const auto cti_request=g_cti_server.take()) {
        // HTTP 202 is merely queued; only the GUI thread can submit SIP.
        const auto observed=current_sip_observation(g_baresip_process.poll());
        const auto grant=axv::authorize_call_command(
            observed,observed.session,g_sip_selection,
            axv::CallAction::Dial,0,cti_request->destination);
        bool submitted=false;
        if (!grant.allowed() || g_shutdown.pending()) {
            SetWindowTextW(g_status,L"CTI: pedido recusado; confira o registro SIP.");
        } else {
            show_page(window,0);
            SetWindowTextW(g_number,grant.destination.c_str());
            // Shared with the manual button: revalidate again at dispatch.
            submitted=try_dial_from_ui(window,grant.destination);
        }
        (void)g_cti_server.complete(cti_request->id,submitted);
    }
 }
#endif



// Collect only existing user selections. Sensitive password remains in the
// original (session-only) edit unless explicitly replaced by the user.
void open_faleweb_preferences(HWND owner) {
    if(g_smoke_mode)return;
    axv::FalewebPreferences prefs{};
    auto text_of=[](HWND control){return axv::pref_edit_text(control);};
    prefs.server=text_of(g_server);
    prefs.user=text_of(g_user);
    prefs.port=text_of(g_port);
#ifdef AXV_ENABLE_NATIVE_SIP
    prefs.remember_password=g_remember_password &&
        SendMessageW(g_remember_password,BM_GETCHECK,0,0)==BST_CHECKED;
    prefs.ca_file=text_of(g_ca_file);
    prefs.mute_ringtone=g_incoming_ringtone_muted;
#endif
    prefs.tls=g_tls && SendMessageW(g_tls,BM_GETCHECK,0,0)==BST_CHECKED;
    prefs.ringtone=g_ringtone_preset.load();
    prefs.codec_mode=g_codec_mode;
    for(const auto& device:g_audio_inputs){
        prefs.inputs.push_back(device.name);
        if(device.id==g_selected_input_id)prefs.input_index=static_cast<int>(prefs.inputs.size())-1;
    }
    for(const auto& device:g_audio_outputs){
        prefs.outputs.push_back(device.name);
        prefs.rings.push_back(device.name);
        if(device.id==g_selected_output_id)prefs.output_index=static_cast<int>(prefs.outputs.size())-1;
        if(device.id==g_selected_ringtone_id)prefs.ring_index=static_cast<int>(prefs.rings.size())-1;
    }
    axv::FalewebIncomingActions incoming{};
#ifdef AXV_ENABLE_NATIVE_SIP
    incoming.context=owner;
    incoming.available=[](void*)->bool{
        return axv::preferences_incoming_controls_available(
            native_engine().observation(),g_displayed_sip);
    };
    incoming.answer=[](void* context){
        if(!axv::preferences_incoming_controls_available(
            native_engine().observation(),g_displayed_sip))return;
        const HWND window=static_cast<HWND>(context);
        // The existing native SIP command handler rechecks call identity.
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kAnswerNative,BN_CLICKED),
            reinterpret_cast<LPARAM>(GetDlgItem(window,kAnswerNative)));
    };
    incoming.reject=[](void* context){
        if(!axv::preferences_incoming_controls_available(
            native_engine().observation(),g_displayed_sip))return;
        const HWND window=static_cast<HWND>(context);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kHangupFuture,BN_CLICKED),
            reinterpret_cast<LPARAM>(GetDlgItem(window,kHangupFuture)));
    };
#endif
    if(!axv::edit_faleweb_preferences(owner,prefs,incoming))return;
    // These changes are applied to existing SIP registration controls. Live
    // registrations are NOT silently restarted or overwritten.
    if(g_server)SetWindowTextW(g_server,prefs.server.c_str());
    if(g_user)SetWindowTextW(g_user,prefs.user.c_str());
    if(g_port)SetWindowTextW(g_port,prefs.port.c_str());
#ifdef AXV_ENABLE_NATIVE_SIP
    if(g_password && !prefs.password.empty())
        SetWindowTextW(g_password,prefs.password.c_str());
    if(g_remember_password) {
        bool remember=prefs.remember_password;
        if(!remember) {
            if(!forget_sip_credential()){
                SetWindowTextW(g_process_status,L"Nao foi possivel excluir a senha do Windows.");
                remember=true;
            } else clear_sip_account_identifiers();
        } else if(!prefs.password.empty() && !save_sip_credential(prefs.password)) {
            SetWindowTextW(g_process_status,L"Falha ao salvar senha SIP no Windows.");
            remember=false;
        } else if(!prefs.password.empty() || sip_credential_exists()) {
            (void)save_sip_account_identifiers();
        }
        SendMessageW(g_remember_password,BM_SETCHECK,
            remember?BST_CHECKED:BST_UNCHECKED,0);
    }
    if(!prefs.password.empty())
        SecureZeroMemory(prefs.password.data(),prefs.password.size()*sizeof(wchar_t));
    prefs.password.clear();
    if(g_ca_file)SetWindowTextW(g_ca_file,prefs.ca_file.c_str());
    if(prefs.mute_ringtone)g_ring_player.stop();
    g_incoming_ringtone_muted=prefs.mute_ringtone;
#endif
    if(g_tls)SendMessageW(g_tls,BM_SETCHECK,prefs.tls?BST_CHECKED:BST_UNCHECKED,0);
    // Only future registrations use this choice. Active calls are unchanged.
    if(axv::valid_native_codec_mode(prefs.codec_mode))g_codec_mode=prefs.codec_mode;
    if(prefs.ringtone>=0 && prefs.ringtone<=5){
        bool allowed=prefs.ringtone!=5;
        if(!allowed){
            std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);
            allowed=!g_ringtone_custom_mp3.empty();
        }
        if(allowed)g_ringtone_preset.store(prefs.ringtone);
    }
    // Device routing changes only while the native engine is stopped.
    bool may_change_devices=true;
#ifdef AXV_ENABLE_NATIVE_SIP
    may_change_devices=native_engine().observation().process==axv::RuntimeProcess::Stopped;
#endif
    if(may_change_devices){
        if(prefs.input_index>=0 &&
           prefs.input_index<static_cast<int>(g_audio_inputs.size())){
            g_selected_input_id=g_audio_inputs[static_cast<size_t>(prefs.input_index)].id;
        }
        if(prefs.output_index>=0 &&
           prefs.output_index<static_cast<int>(g_audio_outputs.size())){
            g_selected_output_id=g_audio_outputs[static_cast<size_t>(prefs.output_index)].id;
        }
    }
    if(prefs.ring_index>=0 &&
       prefs.ring_index<static_cast<int>(g_audio_outputs.size()))
        g_selected_ringtone_id=g_audio_outputs[static_cast<size_t>(prefs.ring_index)].id;
    refresh_account_feedback();
    save_ui_preferences();
    refresh_runtime_banner(g_baresip_process.poll());
}

void show_faleweb_actions(HWND owner){
    if(g_smoke_mode)return;
    HMENU menu=CreatePopupMenu();
    if(!menu)return;
    const struct Action{UINT command;int control;LPCWSTR title;} actions[]={
#ifdef AXV_ENABLE_NATIVE_SIP
        {1,kAnswerNative,L"Atender"},
#endif
        {2,kHangupFuture,L"Encerrar"},
        {3,kTransferFuture,L"Transferir"},
        {4,kHoldFuture,L"Espera / Retomar"},
        {5,kMuteFuture,L"Microfone mudo"},
        {6,kDtmfButton,L"Alternar DTMF"},
        {7,kIncomingRingtoneMuteButton,L"Silenciar toque"},
        {8,kBackspace,L"Apagar digito"},
        {9,kClear,L"Limpar numero"}
    };
    for(const auto& action:actions){
        const HWND control=GetDlgItem(owner,action.control);
        if(!control)continue;
        AppendMenuW(menu,MF_STRING|(IsWindowEnabled(control)?0:MF_GRAYED),
            action.command,action.title);
    }
    RECT anchor{};GetWindowRect(g_faleweb_actions_button,&anchor);
    const UINT chosen=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,
       anchor.left,anchor.bottom,0,owner,nullptr);
    DestroyMenu(menu);
    for(const auto& action:actions)
        if(chosen==action.command)
            SendMessageW(owner,WM_COMMAND,MAKEWPARAM(action.control,BN_CLICKED),
              reinterpret_cast<LPARAM>(GetDlgItem(owner,action.control)));
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_COPYDATA: {
        // Another AXE Voice instance may forward a Windows protocol click.
        // No WM_COPYDATA message is trusted without explicit confirmation.
        const auto* data=reinterpret_cast<const COPYDATASTRUCT*>(lparam);
        if(!data || data->dwData!=kExternalClickDataId || !data->lpData ||
           data->cbData<sizeof(wchar_t)*2 ||
           data->cbData>sizeof(wchar_t)*513 ||
           data->cbData%sizeof(wchar_t) || g_external_click_uri) return FALSE;
        const auto* value=static_cast<const wchar_t*>(data->lpData);
        const auto count=static_cast<std::size_t>(data->cbData/sizeof(wchar_t));
        if(value[count-1]!=L'\0')return FALSE;
        const std::wstring_view uri(value,count-1);
        if(uri.find(L'\0')!=std::wstring_view::npos ||
           !axv::parse_axvoice_click_to_call_uri(uri)) return FALSE;
        g_external_click_uri=std::wstring(uri);
        if(!PostMessageW(window,kExternalClickToCallMessage,0,0)){
            g_external_click_uri.reset();
            return FALSE;
        }
        return TRUE;
    }
    case kExternalClickToCallMessage: {
        if(!g_external_click_uri)return 0;
        auto uri=std::move(*g_external_click_uri);
        g_external_click_uri.reset();
        const auto destination=axv::parse_axvoice_click_to_call_uri(uri);
        if(!destination)return 0;
        if(g_compact){g_compact=false;apply_compact_layout(window);save_ui_preferences();}
        show_page(window,0);
        SetWindowTextW(g_number,destination->c_str());
        ShowWindow(window,SW_RESTORE);
        SetForegroundWindow(window);
        const auto observed=current_sip_observation(g_baresip_process.poll());
        const auto permission=axv::authorize_call_command(
            observed,observed.session,g_sip_selection,axv::CallAction::Dial,0,*destination);
        if(!permission.allowed()){
            SetWindowTextW(g_status,
                L"Click-to-call: destino preenchido. Registre o ramal e confirme CHAMAR.");
            return 0;
        }
        // URLs can be invoked by untrusted websites: always confirm in-app.
        const std::wstring prompt=L"Sistema externo solicita ligar para:\n" +
            permission.destination + L"\n\nDeseja iniciar a chamada?";
        if(MessageBoxW(window,prompt.c_str(),L"Confirmar Click-to-Call",
                       MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES) {
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCallButton,BN_CLICKED),
                         reinterpret_cast<LPARAM>(GetDlgItem(window,kCallButton)));
        } else {
            SetWindowTextW(g_status,L"Click-to-call cancelado; nenhuma chamada solicitada.");
        }
        return 0;
    }
#ifdef AXV_ENABLE_NATIVE_SIP
    case axv::contact_dialer_review_message:
        if(g_compact){g_compact=false;apply_compact_layout(window);save_ui_preferences();}
        show_page(window,0);
        return 0;
#endif
    case WM_CREATE: {
        g_faleweb_mode=!g_smoke_mode || g_faleweb_smoke || g_essential_smoke;
        g_dpi=axv::normalized_dpi(GetDpiForWindow(window));
        load_ui_preferences();
        read_windows_contrast();
        g_font_regular = create_font(15, FW_MEDIUM);
        g_font_title = create_font(27, FW_BOLD);
        g_font_small = create_font(13, FW_NORMAL);
        g_font_digit = create_font(21, FW_SEMIBOLD);
        g_faleweb_digit_font=create_font(15,FW_SEMIBOLD);
        g_faleweb_letters_font=create_font(9,FW_NORMAL);
        refresh_brushes();
        g_brand_label=make_label(window, L"AXE Voice", 24, 16, 240, 37, -1, g_font_title);
        make_label(window, L"SOFTPHONE DESKTOP  /  LAB", 25, 57, 270, 22, -1, g_font_small);
        make_button(window, g_compact ? L"MAX" : L"MINI", kCompact, 344, 65, 70, 29);
        make_button(window, L"?", kHelpButton, 301, 65, 34, 29);
        make_button(window, g_pinned ? L"SOLTAR" : L"FIXAR", kPin, 265, 18, 70, 36);
        make_button(window, g_faleweb_mode && !g_faleweb_smoke ? (g_dark ? L"☀" : L"☾") : (g_dark ? L"CLARO" : L"ESCURO"), kTheme, 344, 18, 70, 36);
        if (g_pinned) SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
                                   SWP_NOMOVE | SWP_NOSIZE);
        make_button(window, L"TECLADO", kShowDial, 24, 103, 92, 42);
        make_button(window, L"AJUSTES", kShowSettings, 123, 103, 92, 42);
        make_button(window, L"AUDIO", kShowAudio, 222, 103, 92, 42);
        make_button(window, L"MONITOR", kShowSipMonitor, 321, 103, 93, 42);
        if(g_faleweb_mode){
            g_faleweb_prefs_button=make_button(window,L"PREFERENCIAS",kFalewebPrefsButton,10,121,113,26);
            g_faleweb_actions_button=make_button(window,L"FUNCOES",kFalewebActionsButton,128,121,112,26);
        }
        g_runtime_banner = make_label(window, L"MOTOR PARADO  |  SIP NAO REGISTRADO",
                                     42, 164, 347, 25, -1, g_font_regular, kRuntimeStatusLabel);

#ifdef AXV_ENABLE_NATIVE_SIP
        g_call_duration_label = make_label(window, L"DURACAO --:--:--",
                                          42, 190, 187, 20, -1, g_font_small, kCallDurationLabel);
        g_ringtone_status_label = make_label(window, L"",
                                             239, 190, 150, 20, -1, g_font_small, kRingtoneStatusLabel);
#endif

#ifdef AXV_ENABLE_NATIVE_SIP
        make_label(window, L"DISCAGEM", 39, 220, 100, 23, 0, g_font_small);
        make_button(window,L"CONTATOS",kContactsButton,154,216,108,28,0);
#else
        make_label(window, L"DISCAGEM", 39, 220, 200, 23, 0, g_font_small);
#endif
#ifdef AXV_ENABLE_NATIVE_SIP
        make_button(window,L"RECENTES",kRecentDestinationsButton,270,216,119,28,0);
        EnableWindow(GetDlgItem(window,kRecentDestinationsButton),FALSE);
#endif
        g_number_label = make_label(window, L"Número ou endereço SIP", 39, 246, 320, 20, 0, g_font_regular);
        g_number = make_edit(window, L"", kNumberEdit, 39, 272, 350, 43, 0);

        constexpr const wchar_t* dial_labels[] = {
            L"1",L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"*",L"0",L"#"
        };
        for (int i = 0; i != 12; ++i) {
            const int col = i % 3;
            const int row = i / 3;
            make_button(window, dial_labels[i], kDigitStart + i,
                        39 + col * 118, 329 + row * 48, 114, 44, 0);
        }
        make_button(window, L"APAGAR", kBackspace, 39, 530, 170, 38, 0);
        make_button(window, L"LIMPAR", kClear, 218, 530, 171, 38, 0);
#ifdef AXV_ENABLE_NATIVE_SIP
        make_button(window,L"CHAMAR",kCallButton,39,576,170,46,0);
        make_button(window,L"TRANSFERIR",kTransferFuture,219,576,170,46,0);
        make_button(window,L"DTMF OFF",kDtmfButton,39,685,170,36,0);
        make_button(window,L"TOQUE NORMAL",kIncomingRingtoneMuteButton,219,685,170,36,0);
        EnableWindow(GetDlgItem(window,kDtmfButton),FALSE);
#else
        make_button(window, L"CHAMAR  •  SIP INDISPONÍVEL", kCallButton,
                    39, 576, 350, 46, 0);
#endif
#ifdef AXV_ENABLE_NATIVE_SIP
        make_button(window,L"ATENDER",kAnswerNative,39,638,82,34,0);
#else
        make_button(window, L"MUDO", kMuteFuture, 39, 638, 82, 34, 0);
#endif
        make_button(window, L"ESPERA", kHoldFuture, 127, 638, 82, 34, 0);
#ifdef AXV_ENABLE_NATIVE_SIP
        make_button(window,L"MUDO",kMuteFuture,215,638,82,34,0);
#else
        make_button(window, L"TRANSF.", kTransferFuture, 215, 638, 82, 34, 0);
#endif
        make_button(window, L"ENCERRAR", kHangupFuture, 303, 638, 86, 34, 0);

#ifdef AXV_ENABLE_NATIVE_SIP
        make_label(window,L"CONTA SIP  •  CANDIDATO NATIVO",39,219,361,26,1,g_font_small);
#else
        make_label(window, L"CONFIGURAÇÃO DE CONTA  •  SEM REGISTRO", 39, 219, 361, 26,
                   1, g_font_small);
#endif
        make_label(window, L"Servidor SIP", 39, 255, 340, 20, 1);
        g_server = make_edit(window, L"", kServerEdit, 39, 280, 350, 38, 1);
        make_label(window, L"Usuário SIP", 39, 329, 212, 20, 1);
        make_label(window, L"Porta", 268, 329, 118, 20, 1);
        g_user = make_edit(window, L"", kUserEdit, 39, 355, 216, 38, 1);
        g_port = make_edit(window, L"5060", kPortEdit, 267, 355, 122, 38, 1);
#ifdef AXV_ENABLE_NATIVE_SIP
        restore_sip_account_identifiers();
        make_label(window,L"Senha SIP",39,402,190,20,1);
        g_password=make_edit(window,L"",kPasswordEdit,39,425,350,32,1);
        SetWindowLongPtrW(g_password,GWL_STYLE,GetWindowLongPtrW(g_password,GWL_STYLE)|ES_PASSWORD);
        SendMessageW(g_password,EM_SETPASSWORDCHAR,L'*',0);
        SendMessageW(g_password,EM_SETLIMITTEXT,256,0);
        g_remember_password=make_control(window,L"BUTTON",L"Salvar senha SIP",
            WS_TABSTOP|BS_AUTOCHECKBOX|BS_NOTIFY,232,398,157,24,kRememberSipPassword,1);
        SendMessageW(g_remember_password,BM_SETCHECK,
            sip_credential_exists()?BST_CHECKED:BST_UNCHECKED,0);
        g_tls=make_control(window,L"BUTTON",L"TLS: exige CA confiavel; nao criptografa RTP",WS_TABSTOP|BS_AUTOCHECKBOX|BS_NOTIFY,39,464,350,28,kTlsCheckbox,1);
        make_label(window,L"Arquivo CA PEM (obrigatorio para TLS)",39,497,350,20,1);
        g_ca_file=make_edit(window,L"",kCaEdit,39,520,350,32,1);
        make_button(window,L"VALIDAR DADOS",kValidateAccount,39,565,350,32,1);
#else
        g_tls = make_control(window, L"BUTTON", L"Usar TLS (quando suportado)",
                             WS_TABSTOP | BS_AUTOCHECKBOX | BS_NOTIFY, 39, 404, 340, 28,
                             kTlsCheckbox, 1);
        make_button(window, L"VALIDAR DADOS  /  SEM REGISTRAR", kValidateAccount,
                    39, 440, 350, 38, 1);
        make_label(window, L"DIAGNÓSTICOS LOCAIS", 39, 492, 331, 19, 1, g_font_small);
        make_button(window, L"DISPOSITIVOS DE ÁUDIO", kAudioDevicesButton,
                    39, 517, 350, 36, 1);
        make_button(window, L"VERIFICAR BARESIP", kProbeButton,
                    39, 561, 350, 36, 1);
#endif
        make_button(window, L"INICIAR MOTOR", kStartBaresipButton,
                    39, 605, 170, 39, 1);
        make_button(window, L"PARAR MOTOR", kStopBaresipButton,
                    218, 605, 171, 39, 1);
#ifdef AXV_ENABLE_NATIVE_SIP
        make_button(window,L"DISPOSITIVOS DE AUDIO",kAudioDevicesButton,39,654,350,36,1);
        g_audio_status=make_label(window,L"Audio nao inspecionado.",39,750,350,40,1,g_font_small);
#else
        g_audio_status = make_label(window, L"Áudio não inspecionado.",
                                    39, 654, 350, 41, 1, g_font_small);
#endif
        g_process_status = make_label(window, L"Baresip parado; registro SIP indisponível.",
                                      39, 702, 350, 43, 1, g_font_small);
        make_label(window,L"MONITOR SIP  /  EVENTOS OBSERVADOS",39,219,355,26,3,g_font_small);
        make_label(window,L"Nao captura mensagens SIP brutas; sem senhas ou numeros.",39,252,355,40,3,g_font_small);
        g_monitor_events=make_control(window,L"EDIT",L"Aguardando eventos do motor...",
            WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
            39,306,350,390,0,3,g_font_small);
        make_label(window,L"Historico temporario: ate 80 transicoes de estado.",39,706,352,28,3,g_font_small);
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
                    39, 507, 82, 43, 2);
        make_button(window, L"TOQUE", kTestRingtoneButton,
                    126, 507, 84, 43, 2);
        make_button(window, L"VOZ", kTestConversationButton,
                    215, 507, 84, 43, 2);
        make_button(window, L"MICROFONE", kTestMicrophoneButton,
                    304, 507, 85, 43, 2);
        make_label(window,L"ESTILO DO TOQUE DE CHAMADA",39,560,350,20,2,g_font_small);
        g_ringtone_preset_combo=make_control(window,L"COMBOBOX",L"",
            WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST,
            39,583,350,200,kRingtonePresetCombo,2);
        const wchar_t* presets[]={
            L"1 - Classico",L"2 - Suave",L"3 - Padrao",
            L"4 - Moderno",L"5 - Agudo",L"Personalizado (MP3)"};
        for(const auto* name:presets)
            SendMessageW(g_ringtone_preset_combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
        SendMessageW(g_ringtone_preset_combo,CB_SETCURSEL,g_ringtone_preset.load(),0);
        make_button(window,L"SELECIONAR MP3",kRingtoneChooseMp3,39,620,350,32,2);
        g_headset_status=make_label(window,
            L"Toques internos: saida escolhida. MP3: saida padrao Windows.",
            39,658,350,30,2,g_font_small);
        // The audio explanation is displayed in the fixed footer, not among
        // the audio controls. The hidden label retains worker feedback text.
        ShowWindow(g_headset_status,SW_HIDE);
        make_button(window, L"PRONTIDAO (F2)",
                    kReadinessButton, 39, 690, 169, 42, 2);
        make_button(window, L"COPIAR STATUS",kCopyStatusButton,219,690,170,42,2);
        g_footer_panel=CreateWindowExW(0,L"STATIC",L"",
            WS_CHILD|WS_VISIBLE|SS_LEFT,0,0,1,1,window,nullptr,
            GetModuleHandleW(nullptr),nullptr);
        g_status = make_label(window, L"Digite um ramal ou endereço SIP.",
                              24, kFullFeedbackY, 390, 30, -1, g_font_small, kFeedbackLabel);
        position_footer(window);
        refresh_destination_feedback();

        EnableWindow(g_input_audio_combo, FALSE);
        EnableWindow(g_output_audio_combo, FALSE);
        EnableWindow(g_ringtone_audio_combo, FALSE);
        EnableWindow(GetDlgItem(window, kTestRingtoneButton), FALSE);
        EnableWindow(GetDlgItem(window, kTestConversationButton), FALSE);
        EnableWindow(GetDlgItem(window, kTestMicrophoneButton), FALSE);
        // A clear disabled affordance: no real SIP engine has been connected yet.
        for (int id : {kCallButton, kMuteFuture, kHoldFuture,
                       kTransferFuture, kHangupFuture}) {
            EnableWindow(GetDlgItem(window, id), FALSE);
        }
#ifdef AXV_ENABLE_NATIVE_SIP
        SetWindowTextW(GetDlgItem(window,kStartBaresipButton),L"REGISTRAR");
        SetWindowTextW(g_process_status,L"Candidato nativo. Conta nao registrada; midia nao homologada.");
#endif
        show_page(window, 0);
        apply_compact_layout(window);
        refresh_runtime_banner(g_baresip_process.poll());
        SetTimer(window, kProcessTimer, 100, nullptr);
        g_controls_ready = true;
        sync_viewport_scroll(window);
        if(g_faleweb_mode && !g_smoke_mode){
            g_audio_change_policy.enter(GetTickCount64());
            request_audio_panel(); // asynchronous WASAPI inventory for Preferences.
        }
        return 0;
    }
    case WM_COMMAND:
        if (lparam && (HIWORD(wparam) == EN_SETFOCUS ||
                       HIWORD(wparam) == BN_SETFOCUS ||
                       HIWORD(wparam) == CBN_SETFOCUS))
            bring_focused_control_into_view(window, reinterpret_cast<HWND>(lparam));
        if (LOWORD(wparam) == kNumberEdit && HIWORD(wparam) == EN_CHANGE) {
            refresh_destination_feedback(); // keyboard, paste and dialpad share validation.
            return 0;
        }
        if (HIWORD(wparam) == EN_CHANGE &&
            (LOWORD(wparam) == kServerEdit || LOWORD(wparam) == kUserEdit ||
             LOWORD(wparam) == kPortEdit)) {
#ifdef AXV_ENABLE_NATIVE_SIP
            // A changed identity must never reuse a password left in the
            // edit control for a different SIP server/user combination.
            if(LOWORD(wparam)==kServerEdit || LOWORD(wparam)==kUserEdit){
                if(g_password && GetWindowTextLengthW(g_password)>0)
                    SetWindowTextW(g_password,L"");
                if(g_remember_password)
                    SendMessageW(g_remember_password,BM_SETCHECK,
                        sip_credential_exists()?BST_CHECKED:BST_UNCHECKED,0);
            }
#endif
            if (g_page == 1) refresh_account_feedback();
            return 0;
        }
        // Combo selection uses code 1 too; accelerator commands have lParam == 0.
        if (HIWORD(wparam) == BN_CLICKED || (HIWORD(wparam) == 1 && lparam == 0)) {
            const int control = LOWORD(wparam);
            if(control==kFalewebPrefsButton){open_faleweb_preferences(window);return 0;}
            if(control==kFalewebActionsButton){show_faleweb_actions(window);return 0;}
            if(control==kShowSipMonitor){
                if(g_faleweb_mode && !g_faleweb_smoke){
                    show_page(window,g_page==3?0:3);
                    return 0;
                }
                if(g_compact){g_compact=false;apply_compact_layout(window);save_ui_preferences();}
                show_page(window,3);return 0;
            }
            if (control == kShowDial || control == kFocusNumber) { show_page(window, 0); return 0; }
            if(control==kCopyStatusButton){
                const auto status=collect_call_readiness();
                if(g_smoke_mode){
                    const auto report=axv::format_diagnostic_export(status,collect_security_status());
                    g_copy_requested_smoke=report.find(L"\r\n")!=report.npos &&
                        report.find(L"Botao Chamar: indisponivel na interface")!=report.npos;
                }else{
                    const bool copied=axv::copy_call_diagnostic(window,status,collect_security_status());
                    SetWindowTextW(g_status,copied?
                        L"Status copiado, sem dados da conta. Permanece na area de transferencia.":
                        L"Nao foi possivel copiar. Tente novamente; a area de transferencia pode estar ocupada.");
                }
                return 0;
            }
            if (control == kReadinessButton) {
                refresh_runtime_banner(g_baresip_process.poll());
                const std::wstring report = make_readiness_report();
                if (g_smoke_mode) {
                    g_readiness_requested_smoke =
                        report.find(L"Botao Chamar: indisponivel na interface") != std::wstring::npos &&
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
                request_audio_panel();
                return 0;
            }
            if (control == kShowAudio) {
                if (g_compact) {
                    g_compact = false;
                    apply_compact_layout(window);
                    save_ui_preferences();
                }
                show_page(window, 2);
                request_audio_panel();
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
                if (g_compact) show_page(window, 0);
                apply_compact_layout(window);
                save_ui_preferences();
                return 0;
            }
            if (control == kTheme) {
                g_dark = !g_dark;
                SetWindowTextW(GetDlgItem(window, kTheme), g_faleweb_mode && !g_faleweb_smoke ? (g_dark ? L"☀" : L"☾") : (g_dark ? L"CLARO" : L"ESCURO"));
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
#ifdef AXV_ENABLE_NATIVE_SIP
            if(control==kContactsButton){
                if(g_compact){g_compact=false;apply_compact_layout(window);save_ui_preferences();}
                show_page(window,0);
                axv::show_contact_book(window,g_number);return 0;
            }
            if(control==kRecentDestinationsButton){show_recent_destinations(window);return 0;}
            if(control==kIncomingRingtoneMuteButton){
                g_incoming_ringtone_muted=!g_incoming_ringtone_muted;
                // Cancel promptly, without acknowledging or rejecting the call.
                if(g_incoming_ringtone_muted)g_ring_player.stop();
                refresh_runtime_banner(g_baresip_process.poll());
                SetWindowTextW(g_status,g_incoming_ringtone_muted
                    ?L"Som das chamadas recebidas silenciado. Aviso visual e ATENDER continuam disponiveis."
                    :L"Toque recebido em modo normal. Chamadas ainda pendentes podem voltar a tocar.");
                return 0;
            }
            if(control==kDtmfButton){
                const auto call=native_engine().observation();
                if(!g_dtmf_mode.toggle(g_displayed_media,call))
                    SetWindowTextW(g_status,L"DTMF indisponivel: chamada exibida deve estar ativa, sem espera ou transferencia.");
                else SetWindowTextW(g_status,g_dtmf_mode.armed()?L"DTMF via SIP INFO: use o teclado na tela. Aceite da central pendente.":L"DTMF desligado; teclado edita o destino.");
                refresh_runtime_banner(g_baresip_process.poll());return 0;
            }
#endif
            if (control >= kDigitStart && control < kDigitStart + 12) {
#ifdef AXV_ENABLE_NATIVE_SIP
                if(g_dtmf_mode.armed()){
                    const auto key=axv::dialpad_keys()[control-kDigitStart];
                    const auto expected=g_displayed_media;
                    const auto call=native_engine().observation();
                    const bool accepted=g_dtmf_mode.active(expected,call) && native_engine().send_dtmf(key,expected);
                    SetWindowTextW(g_status,accepted?L"Pedido DTMF via INFO enfileirado; recebimento pela central nao confirmado.":L"DTMF recusado: chamada mudou, esta em espera/transferencia ou comando ocupado. Tente novamente.");
                    return 0;
                }
#endif
                edit_number(window, axv::dialpad_keys()[control - kDigitStart]);
                return 0;
            }
            if (control == kBackspace) { edit_number(window, L'\b'); return 0; }
            if (control == kClear) { edit_number(window, L'\0'); return 0; }
        }
#ifdef AXV_ENABLE_NATIVE_SIP
        if(LOWORD(wparam)==kTransferFuture && HIWORD(wparam)==BN_CLICKED){
            const auto expected=g_displayed_media;
            const auto call=native_engine().observation();
            wchar_t target[129]{};GetWindowTextW(g_number,target,129);
            if(!axv::native_media_controls_current(expected,call) || expected.transfer!=axv::NativeTransferState::None || !axv::evaluate_destination(target).valid()){
                SetWindowTextW(g_status,L"Transferencia indisponivel. Informe destino valido em uma chamada estabelecida.");return 0;
            }
            const std::wstring review=L"Transferir esta chamada para:\n\n"+std::wstring(target)+L"\n\nTransferencia direta, sem consulta previa.";
            if(MessageBoxW(window,review.c_str(),L"Revisar destino da transferencia",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return 0;
            // The captured displayed identity remains fixed across the modal dialog.
            if(!native_engine().transfer_call(target,expected)){
                const auto feedback=std::wstring(L"Transferencia recusada. ")+native_engine().diagnostic();
                SetWindowTextW(g_status,feedback.c_str());
            }
            return 0;
        }
        if((LOWORD(wparam)==kMuteFuture || LOWORD(wparam)==kHoldFuture) && HIWORD(wparam)==BN_CLICKED){
            const auto media=g_displayed_media; // Bind intent to what the user actually saw.
            const auto call=native_engine().observation();
            bool accepted=false;
            if(axv::native_media_controls_current(media,call))
                accepted=LOWORD(wparam)==kMuteFuture?native_engine().set_muted(!media.muted,media):native_engine().set_held(!media.local_hold,media);
            if(!accepted)SetWindowTextW(g_status,L"Controle indisponivel para a chamada exibida.");
            return 0;
        }
        if(LOWORD(wparam)==kAnswerNative && HIWORD(wparam)==BN_CLICKED) {
            acknowledge_incoming_alert();
            (void)native_voice_service().answer(g_displayed_sip); return 0;
        }
        if(LOWORD(wparam)==kHangupFuture && HIWORD(wparam)==BN_CLICKED) {
            acknowledge_incoming_alert();
            (void)native_voice_service().hangup(g_displayed_sip); return 0;
        }
#endif
        #ifdef AXV_ENABLE_NATIVE_SIP
        if(LOWORD(wparam)==kRememberSipPassword && HIWORD(wparam)==BN_CLICKED){
            const bool checked=SendMessageW(g_remember_password,BM_GETCHECK,0,0)==BST_CHECKED;
            if(!checked){
                if(!forget_sip_credential()) {
                    SendMessageW(g_remember_password,BM_SETCHECK,BST_CHECKED,0);
                    SetWindowTextW(g_process_status,L"Nao foi possivel excluir a senha salva do Windows.");
                } else clear_sip_account_identifiers();
            } else {
                auto secret=axv::pref_edit_text(g_password);
                if(!secret.empty()){
                    if(!save_sip_credential(secret)){
                        SendMessageW(g_remember_password,BM_SETCHECK,BST_UNCHECKED,0);
                        SetWindowTextW(g_process_status,L"Senha nao salva; confira servidor e usuario SIP.");
                    } else (void)save_sip_account_identifiers();
                }
                if(!secret.empty())SecureZeroMemory(secret.data(),secret.size()*sizeof(wchar_t));
            }
            return 0;
        }
#endif
        if (LOWORD(wparam) == kStartBaresipButton && HIWORD(wparam) == BN_CLICKED) {
#ifdef AXV_ENABLE_NATIVE_SIP
            bool save_failed=false;
            if(g_remember_password &&
               SendMessageW(g_remember_password,BM_GETCHECK,0,0)==BST_CHECKED){
                auto secret=axv::pref_edit_text(g_password);
                if(secret.empty()){
                    secret=load_sip_credential();
                    if(!secret.empty())SetWindowTextW(g_password,secret.c_str());
                } else if(!save_sip_credential(secret)){
                    save_failed=true;
                    SendMessageW(g_remember_password,BM_SETCHECK,BST_UNCHECKED,0);
                } else (void)save_sip_account_identifiers();
                if(!secret.empty())SecureZeroMemory(secret.data(),secret.size()*sizeof(wchar_t));
            }
            const auto result=axv::register_native_account_ui(native_engine(),
                {g_server,g_user,g_port,g_password,g_ca_file,g_tls},g_selected_input_id,g_selected_output_id,g_codec_mode);
            const auto feedback=std::wstring(axv::native_registration_result_message(result))+
                (save_failed?L" Aviso: senha nao foi salva no Windows.":L"");
            SetWindowTextW(g_process_status,feedback.c_str());
            return 0;
#endif
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
#ifdef AXV_ENABLE_NATIVE_SIP
            acknowledge_incoming_alert();g_tone_preview.request_stop();
            native_engine().stop(); refresh_runtime_banner(axv::ProcessStatus::Stopped);
            SetWindowTextW(g_process_status,L"Motor nativo parado; senha descartada."); return 0;
#endif
            if (g_shutdown.pending()) return 0;
            if (g_baresip_process.poll() != axv::ProcessStatus::Running) {
                SetWindowTextW(g_process_status, L"Motor ja estava parado.");
                return 0;
            }
            begin_shutdown(window);
            return 0;
        }
        if (LOWORD(wparam) == kRefreshHeadsetsButton && HIWORD(wparam) == BN_CLICKED) {
            request_audio_panel();
            return 0;
        }
        if (HIWORD(wparam) == CBN_SELCHANGE) {
            const int id = LOWORD(wparam);
            if (id == kInputAudioCombo || id == kOutputAudioCombo ||
                id == kRingtoneAudioCombo) {
#ifdef AXV_ENABLE_NATIVE_SIP
                if(id!=kRingtoneAudioCombo && native_engine().observation().process!=axv::RuntimeProcess::Stopped)return 0;
#endif
                HWND combo = id == kInputAudioCombo ? g_input_audio_combo :
                             id == kOutputAudioCombo ? g_output_audio_combo : g_ringtone_audio_combo;
                const auto& devices = id == kInputAudioCombo ? g_audio_inputs : g_audio_outputs;
                LRESULT index = SendMessageW(combo, CB_GETCURSEL, 0, 0);
                std::wstring& selected = id == kInputAudioCombo ? g_selected_input_id :
                    id == kOutputAudioCombo ? g_selected_output_id : g_selected_ringtone_id;
#ifdef AXV_ENABLE_NATIVE_SIP
                if(index==0){
                    selected.clear();
                    SetWindowTextW(g_headset_status,id==kRingtoneAudioCombo?L"Toque segue o padrao multimedia do Windows.":L"Padrao de comunicacao sera aplicado ao registrar.");
                    refresh_audio_test_actions(window);return 0;
                }
                --index;
#endif
                if (index >= 0 && static_cast<std::size_t>(index) < devices.size()) {
                    selected = devices[static_cast<std::size_t>(index)].id;
                    const auto routing = axv::preview_audio_routing(
                        g_audio_inputs, g_audio_outputs,
                        g_selected_input_id, g_selected_output_id, g_selected_ringtone_id);
#ifdef AXV_ENABLE_NATIVE_SIP
                    SetWindowTextW(g_headset_status,L"Microfone/voz aplicados ao registrar; toque independente para chamadas recebidas.");
#else
                    SetWindowTextW(g_headset_status,
                        axv::audio_routing_preview_feedback(routing));
#endif
                    refresh_audio_test_actions(window);
                }
                return 0;
            }
        }
        if (LOWORD(wparam)==kRingtonePresetCombo && HIWORD(wparam)==CBN_SELCHANGE) {
            const int selected=static_cast<int>(SendMessageW(g_ringtone_preset_combo,CB_GETCURSEL,0,0));
            if(selected>=0 && selected<=5){
                std::wstring custom;
                {std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);custom=g_ringtone_custom_mp3;}
                if(selected==5 && custom.empty()){
                    SetWindowTextW(g_headset_status,L"Escolha um MP3 antes de ativar o toque personalizado.");
                    SendMessageW(g_ringtone_preset_combo,CB_SETCURSEL,g_ringtone_preset.load(),0);
                    return 0;
                }
                g_tone_preview.request_stop();
#ifdef AXV_ENABLE_NATIVE_SIP
                g_ring_player.stop();
#endif
                g_ringtone_preset.store(selected);
                save_ui_preferences();
                refresh_audio_test_actions(window);
                SetWindowTextW(g_headset_status,selected==5
                    ? L"MP3: saida padrao multimidia do Windows."
                    : L"Toque interno: dispositivo selecionado.");
                refresh_runtime_banner(g_baresip_process.poll());
            }
            return 0;
        }
        if(LOWORD(wparam)==kRingtoneChooseMp3 && HIWORD(wparam)==BN_CLICKED){
            if(g_smoke_mode)return 0;
            wchar_t filename[2049]{};
            OPENFILENAMEW picker{};
            picker.lStructSize=sizeof(picker);
            picker.hwndOwner=window;
            picker.lpstrFile=filename;
            picker.nMaxFile=2049;
            picker.lpstrFilter=L"Arquivos MP3 (*.mp3)\0*.mp3\0\0";
            picker.nFilterIndex=1;
            picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
            if(GetOpenFileNameW(&picker)){
                std::wstring chosen=filename;
                const auto dot=chosen.find_last_of(L'.');
                if(dot==chosen.npos || _wcsicmp(chosen.c_str()+dot,L".mp3")!=0){
                    SetWindowTextW(g_headset_status,L"Formato invalido. Escolha arquivo .mp3.");
                    return 0;
                }
                g_tone_preview.request_stop();
#ifdef AXV_ENABLE_NATIVE_SIP
                g_ring_player.stop();
#endif
                {std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);
                 g_ringtone_custom_mp3=std::move(chosen);}
                g_ringtone_preset.store(5);
                SendMessageW(g_ringtone_preset_combo,CB_SETCURSEL,5,0);
                save_ui_preferences();
                refresh_audio_test_actions(window);
                SetWindowTextW(g_headset_status,L"MP3 selecionado. Saida padrao multimidia Windows.");
                refresh_runtime_banner(g_baresip_process.poll());
            }
            return 0;
        }
        if (LOWORD(wparam) == kTestRingtoneButton && HIWORD(wparam) == BN_CLICKED) {
            // Explicit manual preview only; never overlap a verified incoming alert.
            if(!can_preview_selected_ringtone()){
                SetWindowTextW(g_headset_status,L"Previa indisponivel: confira a saida ou a chamada recebida.");
            }else{
#ifdef AXV_ENABLE_NATIVE_SIP
                const bool multimedia_default=true;
#else
                const bool multimedia_default=false;
#endif
                std::wstring mp3;
                {std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);mp3=g_ringtone_custom_mp3;}
                if(g_tone_preview.start_ringtone(g_selected_ringtone_id,multimedia_default,
                       g_ringtone_preset.load(),mp3)){
                    g_preview_is_conversation=false;
                    g_last_tone_state=axv::TonePreviewState::Playing;
                    SetWindowTextW(g_headset_status,axv::tone_preview_feedback(g_last_tone_state));
                    refresh_audio_test_actions(window);
                }
            }
            return 0;
        }
        if (LOWORD(wparam) == kTestConversationButton && HIWORD(wparam) == BN_CLICKED) {
            if (!can_preview_selected_conversation()) {
                SetWindowTextW(g_headset_status,
                    L"Teste de voz indisponivel: confira a saida e encerre chamadas ativas.");
            } else {
#ifdef AXV_ENABLE_NATIVE_SIP
                constexpr bool follow_default = true;
#else
                constexpr bool follow_default = false;
#endif
                const auto endpoint = axv::audio_output_preview_endpoint(
                    g_audio_outputs,g_selected_output_id,follow_default,
                    axv::AudioOutputPreviewRole::Conversation);
                if (endpoint && g_tone_preview.start(*endpoint,false)) {
                    g_preview_is_conversation=true;
                    g_last_tone_state=axv::TonePreviewState::Playing;
                    SetWindowTextW(g_headset_status,
                        L"Testando a saida de conversa no Windows; sem chamada SIP.");
                    refresh_audio_test_actions(window);
                }
            }
            return 0;
        }
        if (LOWORD(wparam) == kTestMicrophoneButton && HIWORD(wparam) == BN_CLICKED) {
            // Explicit, transient capture: no recording or network transfer.
            if (!can_test_selected_microphone()) {
                SetWindowTextW(g_headset_status,
                    L"Microfone indisponivel: atualize dispositivos ou encerre chamadas.");
            } else {
#ifdef AXV_ENABLE_NATIVE_SIP
                constexpr bool follow_default = true;
#else
                constexpr bool follow_default = false;
#endif
                const auto endpoint = axv::microphone_test_endpoint(
                    g_audio_inputs,g_selected_input_id,follow_default);
                if (endpoint && g_microphone_test.start(*endpoint)) {
                    g_last_microphone_state = axv::MicrophoneTestState::Capturing;
                    SetWindowTextW(g_headset_status,
                        axv::microphone_test_feedback(g_last_microphone_state));
                    refresh_audio_test_actions(window);
                }
            }
            return 0;
        }
        if (LOWORD(wparam) == kAudioDevicesButton && HIWORD(wparam) == BN_CLICKED) {
            if (!g_smoke_mode && g_page==1 && !g_compact) {
                g_settings_audio_worker.request();
                SetWindowTextW(g_audio_status,L"Consultando dispositivos de audio...");
            }
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
            // Re-check the active native SIP session on every click.
            wchar_t number[256] = {};
            GetWindowTextW(g_number,number,256);
            (void)try_dial_from_ui(window,std::wstring(number));
            return 0;
        }
        break;
    case WM_DPICHANGED: {
        g_scroll_offset = 0; // pixel offsets are invalid after DPI transitions.
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
        if(g_faleweb_mode)return 0;
        if (!g_controls_ready) return 0;
        RECT client{};
        GetClientRect(window, &client);
        const int page = (std::max)(1, static_cast<int>(client.bottom-client.top));
        int desired = g_scroll_offset;
        switch (LOWORD(wparam)) {
        case SB_TOP: desired = 0; break;
        case SB_BOTTOM: desired = g_scroll_max; break;
        case SB_LINEUP: desired -= axv::dpi_pixels(42,g_dpi); break;
        case SB_LINEDOWN: desired += axv::dpi_pixels(42,g_dpi); break;
        case SB_PAGEUP: desired -= page; break;
        case SB_PAGEDOWN: desired += page; break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK: {
            SCROLLINFO track{};
            track.cbSize = sizeof(track);
            track.fMask = SIF_TRACKPOS;
            GetScrollInfo(window, SB_VERT, &track);
            desired = track.nTrackPos;
            break;
        }
        default: return 0;
        }
        // Dragging can generate hundreds of requests. Keep only the newest
        // thumb position and drain it through one posted GUI message.
        if(LOWORD(wparam)==SB_THUMBTRACK){
            g_pending_scroll_offset=desired;
            if(!g_scroll_message_queued){
                g_scroll_message_queued=PostMessageW(window,kAsyncScrollMessage,0,0)!=FALSE;
                if(!g_scroll_message_queued)set_viewport_scroll(window,desired);
            }
        }else{
            g_scroll_message_queued=false;
            set_viewport_scroll(window,desired);
        }
        return 0;
    }
    case kAsyncCtiMessage:
#ifdef AXV_ENABLE_NATIVE_SIP
        if(!g_shutdown.pending())dispatch_pending_cti(window);
#endif
        return 0;
    case kAsyncScrollMessage:
        if(g_scroll_message_queued){
            g_scroll_message_queued=false;
            set_viewport_scroll(window,g_pending_scroll_offset);
        }
        return 0;
    case WM_MOUSEWHEEL: {
        if(g_faleweb_mode)return 0;
        g_wheel_remainder += GET_WHEEL_DELTA_WPARAM(wparam);
        const int steps = g_wheel_remainder / WHEEL_DELTA;
        g_wheel_remainder %= WHEEL_DELTA;
        if (steps != 0)
            set_viewport_scroll(window, g_scroll_offset -
                                steps * axv::dpi_pixels(48,g_dpi));
        return 0;
    }
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED) {
            if(g_faleweb_mode)apply_faleweb_layout(window);
            position_footer(window);
            sync_viewport_scroll(window);
        }
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
                if (choice == kTrayExit) SendMessageW(window, WM_CLOSE, 0, 0);
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
        const HWND label=reinterpret_cast<HWND>(lparam);
        COLORREF foreground=text_color();
#ifdef AXV_ENABLE_NATIVE_SIP
        if(label==g_ringtone_status_label && g_incoming_ringtone_muted){
            // Explicit warning without overriding the Windows high-contrast palette.
            foreground=g_high_contrast ? GetSysColor(COLOR_WINDOWTEXT)
                      : (g_dark ? RGB(255,206,107) : RGB(139,86,0));
        }
#endif
        SetTextColor(dc,foreground);
        if(g_faleweb_mode && label==g_runtime_banner && !g_high_contrast){
            const auto index=static_cast<int>(g_essential_signal);
            const COLORREF bg[3]={
                g_dark?RGB(71,31,39):RGB(251,232,235),
                g_dark?RGB(69,50,18):RGB(255,246,223),
                g_dark?RGB(23,62,45):RGB(225,247,233)
            };
            const COLORREF fg[3]={
                g_dark?RGB(255,220,225):RGB(132,29,42),
                g_dark?RGB(255,228,160):RGB(118,75,18),
                g_dark?RGB(209,249,222):RGB(22,94,56)
            };
            SetBkColor(dc,bg[index]);
            SetTextColor(dc,fg[index]);
            SetBkMode(dc,OPAQUE);
            return reinterpret_cast<LRESULT>(g_essential_status_brush[index]);
        }
        if(g_faleweb_mode && (label==g_runtime_banner
#ifdef AXV_ENABLE_NATIVE_SIP
           || label==g_call_duration_label || label==g_ringtone_status_label
#endif
           )){
            SetBkColor(dc,g_high_contrast?background_color():
                (g_dark?RGB(19,48,86):RGB(225,242,255)));
            SetTextColor(dc,g_high_contrast?text_color():
                (g_dark?RGB(239,247,255):RGB(13,44,81)));
            SetBkMode(dc,OPAQUE);
            return reinterpret_cast<LRESULT>(g_faleweb_display_brush);
        }
        if(label==g_footer_panel || label==g_status){
            SetBkColor(dc,field_color());
            SetBkMode(dc,OPAQUE);
            return reinterpret_cast<LRESULT>(g_field_brush);
        }
        SetBkMode(dc,TRANSPARENT);
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
        PAINTSTRUCT paint{};
        const HDC window_dc=BeginPaint(window,&paint);
        RECT rect{};
        GetClientRect(window,&rect);
        // Offscreen painting avoids flashing the background between panels.
        const int width=rect.right-rect.left,height=rect.bottom-rect.top;
        HDC back_dc=CreateCompatibleDC(window_dc);
        HBITMAP bitmap=(back_dc && width>0 && height>0)
            ? CreateCompatibleBitmap(window_dc,width,height):nullptr;
        HGDIOBJ old_bitmap=bitmap?SelectObject(back_dc,bitmap):nullptr;
        HDC dc=old_bitmap?back_dc:window_dc;
        FillRect(dc,&rect,g_background_brush);
        if(g_faleweb_mode){
            const auto outer=g_faleweb_smoke?logical_rect(7,7,247,116):
                logical_rect(6,6,350,91);
            draw_card(dc,outer,g_high_contrast?background_color():
                (g_dark?RGB(19,48,86):RGB(225,242,255)),13);
        }else{
        draw_card(dc,logical_rect(24,151,414,198),panel_color(),16);
        draw_card(dc,g_compact?logical_rect(24,206,414,289)
                               :logical_rect(24,206,414,742),panel_color(),18);
        HPEN rule=CreatePen(PS_SOLID,1,line_color());
        HGDIOBJ old=SelectObject(dc,rule);
        const int divider_y=g_compact?294:747;
        MoveToEx(dc,axv::dpi_pixels(24,g_dpi),
                 axv::dpi_pixels(divider_y,g_dpi)-g_scroll_offset,nullptr);
        LineTo(dc,axv::dpi_pixels(414,g_dpi),
               axv::dpi_pixels(divider_y,g_dpi)-g_scroll_offset);
        SelectObject(dc,old);
        DeleteObject(rule);
        }
        if(old_bitmap){
            BitBlt(window_dc,paint.rcPaint.left,paint.rcPaint.top,
                paint.rcPaint.right-paint.rcPaint.left,
                paint.rcPaint.bottom-paint.rcPaint.top,
                back_dc,paint.rcPaint.left,paint.rcPaint.top,SRCCOPY);
            SelectObject(back_dc,old_bitmap);
        }
        if(bitmap)DeleteObject(bitmap);
        if(back_dc)DeleteDC(back_dc);
        EndPaint(window,&paint);
        return 0;
    }
    case WM_DEVICECHANGE:
        if (axv::is_audio_device_change(static_cast<std::uintptr_t>(wparam)))
            audio_devices_changed(GetTickCount64());
        return 0;
    case WM_TIMER:
        if (wparam == kProcessTimer) {
            poll_audio_notifications();
            poll_audio_inventory();
            if(g_page==2 && g_headset_status && g_status){
                wchar_t explanation[512]{},current[512]{};
                GetWindowTextW(g_headset_status,explanation,512);
                GetWindowTextW(g_status,current,512);
                if(std::wcscmp(explanation,current)!=0)SetWindowTextW(g_status,explanation);
            }
            // No Windows UI operations occur on the WASAPI worker thread.
            const auto microphone_state = g_microphone_test.state();
            if (microphone_state != g_last_microphone_state) {
                g_last_microphone_state = microphone_state;
                if (g_headset_status && g_page == 2)
                    SetWindowTextW(g_headset_status,axv::microphone_test_feedback(microphone_state));
                refresh_audio_test_actions(window);
            }
            const auto tone_state = g_tone_preview.state();
            if (tone_state != g_last_tone_state) {
                g_last_tone_state = tone_state;
                if (g_headset_status && g_page == 2)
                    SetWindowTextW(g_headset_status,
                        g_preview_is_conversation && tone_state==axv::TonePreviewState::Played
                            ? L"Teste de voz enviado a saida selecionada. Sem chamada SIP."
                            : axv::tone_preview_feedback(tone_state));
                refresh_audio_test_actions(window);
            }
            refresh_runtime_banner(g_baresip_process.poll());
#ifdef AXV_ENABLE_NATIVE_SIP
            dispatch_pending_cti(window);
#endif
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
#ifdef AXV_ENABLE_NATIVE_SIP
        axv::close_contact_book();
#endif
        g_audio_endpoint_watcher.stop();
        g_audio_worker.close();
        g_settings_audio_worker.close();
        g_tone_preview.request_stop();
        g_microphone_test.request_stop();
#ifdef AXV_ENABLE_NATIVE_SIP
        g_cti_server.stop();
        g_ring_player.stop();
        native_engine().stop();
        (void)g_ring_player.shutdown();
#endif
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
        if(g_faleweb_display_brush)DeleteObject(g_faleweb_display_brush);
        for(HBRUSH brush:g_essential_status_brush)
            if(brush)DeleteObject(brush);
        if(g_faleweb_digit_font)DeleteObject(g_faleweb_digit_font);
        if(g_faleweb_letters_font)DeleteObject(g_faleweb_letters_font);
        // Normally stopped through WM_CLOSE. Forced fallback on external destruction.
        g_baresip_process.stop();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command_line, int show) {
    g_faleweb_smoke=command_line && std::wcscmp(command_line,L"--faleweb-ui-smoke")==0;
    g_essential_smoke=command_line && std::wcscmp(command_line,L"--essential-ui-smoke")==0;
    g_smoke_mode = g_faleweb_smoke || g_essential_smoke || (command_line && (std::wcscmp(command_line, L"--ui-smoke") == 0 ||
                                      std::wcscmp(command_line, L"--ui-blackbox") == 0 || std::wcscmp(command_line,L"--native-ui-smoke")==0));
#ifdef AXV_ENABLE_NATIVE_SIP
    const auto guard_names = axv::native_install_guard_names();
    const auto guard_result = guard_names
        ? g_native_install_guard.acquire(*guard_names)
        : axv::NativeInstallGuardResult::Failed;
    if (guard_result != axv::NativeInstallGuardResult::Acquired) {
        const bool busy = guard_result == axv::NativeInstallGuardResult::Busy;
        if (!g_smoke_mode) {
            MessageBoxW(nullptr,
                busy ? L"O instalador do AXE Voice está em execução. Aguarde sua conclusão e tente novamente."
                     : L"Não foi possível verificar a proteção de instalação do AXE Voice. O aplicativo não será iniciado.",
                L"AXE Voice", MB_OK | MB_ICONWARNING);
        }
        return busy ? ERROR_BUSY : ERROR_ACCESS_DENIED;
    }
#endif
    // Windows URL protocol routes browser clicks to a single running GUI.
    // A second instance forwards only a validated URI, never a SIP command.
    if (!g_smoke_mode && command_line && *command_line) {
        int argc=0;
        LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
        if(argv) {
            if(argc==2 && axv::parse_axvoice_click_to_call_uri(argv[1])) {
                const std::wstring uri=argv[1];
                if(HWND existing=FindWindowW(L"AXEVoiceDesktopWindow",nullptr)) {
                    COPYDATASTRUCT data{};
                    data.dwData=kExternalClickDataId;
                    data.cbData=static_cast<DWORD>((uri.size()+1)*sizeof(wchar_t));
                    data.lpData=const_cast<wchar_t*>(uri.c_str());
                    DWORD_PTR received=0;
                    const auto ok=SendMessageTimeoutW(existing,WM_COPYDATA,0,
                        reinterpret_cast<LPARAM>(&data),SMTO_ABORTIFHUNG|SMTO_BLOCK,
                        2000,&received);
                    LocalFree(argv);
                    return ok && received==TRUE ? 0 : ERROR_BUSY;
                }
                g_external_click_uri=uri;
            }
            LocalFree(argv);
        }
    }
    g_faleweb_mode=!g_smoke_mode || g_essential_smoke;
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
#ifdef AXV_ENABLE_NATIVE_SIP
    const wchar_t* title=L"AXE Voice | Candidato nativo 0.83";
#else
    const wchar_t* title=L"AXE Voice  |  Desktop Lab v0.45";
#endif
    HWND window = CreateWindowExW(0, className, title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | (g_faleweb_mode?0:WS_VSCROLL) | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        g_faleweb_mode?axv::dpi_pixels(g_faleweb_smoke?kFalewebWidth:kEssentialWidth,g_dpi):axv::dpi_pixels(kWindowWidth,g_dpi)+GetSystemMetricsForDpi(SM_CXVSCROLL,g_dpi),
        axv::dpi_pixels(g_faleweb_mode?(g_faleweb_smoke?kFalewebHeight:kEssentialHeight):kFullHeight,g_dpi),
        nullptr, nullptr, instance, nullptr);
    if (!window) return 2;
    if (!g_smoke_mode) (void)g_audio_endpoint_watcher.start();
    if (!g_smoke_mode && g_external_click_uri)
        PostMessageW(window,kExternalClickToCallMessage,0,0);
#ifdef AXV_ENABLE_NATIVE_SIP
    // Enable only by explicit per-process configuration. Never print tokens.
    // The HTTP server binds loopback; a Windows browser cannot use CORS.
    if (!g_smoke_mode) {
        char token[129]{};
        const DWORD count=GetEnvironmentVariableA(
            "AXV_LOCAL_API_TOKEN",token,static_cast<DWORD>(sizeof(token)));
        if (count) {
            if (count < sizeof(token) && g_cti_server.start(std::string(token,count))) {
                g_cti_server.set_notification_target(reinterpret_cast<std::uintptr_t>(window),kAsyncCtiMessage);
                SetWindowTextW(g_status,L"CTI local habilitada (127.0.0.1:27652).");
            } else {
                SetWindowTextW(g_status,L"CTI desabilitada: token invalido ou porta ocupada.");
            }
            SecureZeroMemory(token,sizeof(token));
        }
    }
#endif
    if(g_essential_smoke){
        // Exercise the real owner-drawn Win32 controls and their physical
        // rectangles without registering a SIP account or starting a call.
        bool passed=true;
        std::wstring design_failure;
        const auto report_design_failure=[&](const std::wstring& reason) {
            if(design_failure.empty()) design_failure=reason;
        };
        const auto visible=[](HWND control){
            return control && (GetWindowLongPtrW(control,GWL_STYLE)&WS_VISIBLE)!=0;
        };
        const auto in_client=[&](HWND control){
            if(!control)return false;
            RECT client{},rect{};
            GetClientRect(window,&client);
            POINT tl{client.left,client.top},br{client.right,client.bottom};
            ClientToScreen(window,&tl);ClientToScreen(window,&br);
            GetWindowRect(control,&rect);
            return rect.left>=tl.x && rect.top>=tl.y &&
                   rect.right<=br.x && rect.bottom<=br.y &&
                   rect.right>rect.left && rect.bottom>rect.top;
        };
        // Design acceptance: all visible interactive/identity controls must
        // occupy distinct physical rectangles inside the usable client area.
        // This catches overlap and clipped navigation beyond the older tests
        // which only checked the dial input and primary call action.
        const auto essential_geometry_ok=[&](){
            std::vector<HWND> controls{
                g_brand_label,GetDlgItem(window,kTheme),
                GetDlgItem(window,kShowSipMonitor),g_runtime_banner,g_number,
                GetDlgItem(window,kCallButton),GetDlgItem(window,kHangupFuture),
                GetDlgItem(window,kMuteFuture),GetDlgItem(window,kHoldFuture),
                GetDlgItem(window,kTransferFuture),GetDlgItem(window,kShowDial),
                g_faleweb_prefs_button
            };
#ifdef AXV_ENABLE_NATIVE_SIP
            controls.push_back(GetDlgItem(window,kContactsButton));
            controls.push_back(GetDlgItem(window,kRecentDestinationsButton));
            controls.push_back(GetDlgItem(window,kAnswerNative));
#endif
            for(int i=0;i<12;++i)
                controls.push_back(GetDlgItem(window,kDigitStart+i));
            std::vector<RECT> occupied;
            for(const HWND control:controls){
                if(!control) {report_design_failure(L"Controle obrigatório não existe.");return false;}
                if(!visible(control))continue;
                if(!in_client(control)) {
                    report_design_failure(L"Controle fora da janela: ID "+
                        std::to_wstring(GetDlgCtrlID(control))+
                        L", DPI "+std::to_wstring(g_dpi));
                    return false;
                }
                RECT bounds{};
                if(!GetWindowRect(control,&bounds))return false;
                for(const RECT& other:occupied)
                    if(bounds.left<other.right && other.left<bounds.right &&
                       bounds.top<other.bottom && other.top<bounds.bottom) {
                        report_design_failure(L"Sobreposição: ID "+
                            std::to_wstring(GetDlgCtrlID(control))+
                            L", DPI "+std::to_wstring(g_dpi));
                        return false;
                    }
                occupied.push_back(bounds);
            }
            return occupied.size()>=18 && !visible(g_footer_panel) &&
                !visible(g_status) && g_scroll_max==0 && g_scroll_offset==0 &&
                !(GetWindowLongPtrW(window,GWL_STYLE)&WS_VSCROLL);
        };
        // Owner-drawn action labels should fit without using the truncating
        // DT_END_ELLIPSIS fallback (which hides a design regression).
        const auto essential_labels_fit=[&](){
            std::vector<HWND> labels{
                GetDlgItem(window,kTheme),GetDlgItem(window,kShowSipMonitor),
                GetDlgItem(window,kCallButton),GetDlgItem(window,kHangupFuture),
                GetDlgItem(window,kMuteFuture),GetDlgItem(window,kHoldFuture),
                GetDlgItem(window,kTransferFuture),GetDlgItem(window,kShowDial),
                g_faleweb_prefs_button
            };
#ifdef AXV_ENABLE_NATIVE_SIP
            labels.push_back(GetDlgItem(window,kContactsButton));
            labels.push_back(GetDlgItem(window,kRecentDestinationsButton));
            labels.push_back(GetDlgItem(window,kAnswerNative));
#endif
            for(const HWND control:labels){
                if(!control)return false;
                if(!visible(control))continue;
                wchar_t value[128]{};
                const int length=GetWindowTextW(control,value,128);
                if(length<=0)return false;
                HDC dc=GetDC(control);
                if(!dc)return false;
                const HGDIOBJ old=SelectObject(dc,
                    control==GetDlgItem(window,kTheme) ? g_font_small : g_font_regular);
                SIZE text_size{};
                const bool measured=GetTextExtentPoint32W(dc,value,length,&text_size)!=0;
                SelectObject(dc,old);
                ReleaseDC(control,dc);
                RECT bounds{};
                GetWindowRect(control,&bounds);
                const int horizontal_space= axv::dpi_pixels(8,g_dpi);
                const int vertical_space= axv::dpi_pixels(4,g_dpi);
                if(!measured ||
                   text_size.cx>bounds.right-bounds.left-horizontal_space ||
                   text_size.cy>bounds.bottom-bounds.top-vertical_space) {
                    report_design_failure(L"Texto cortado: "+
                        std::wstring(value)+L", ID "+
                        std::to_wstring(GetDlgCtrlID(control))+
                        L", espaço "+std::to_wstring(bounds.right-bounds.left)+
                        L", texto "+std::to_wstring(text_size.cx)+
                        L", DPI "+std::to_wstring(g_dpi));
                    return false;
                }
            }
            return true;
        };
        const auto apply=[&](axv::SipStatus status,std::uint64_t call,
                             bool verified,axv::EssentialCallView expected){
            axv::SipRuntimeObservation snapshot{axv::RuntimeProcess::Running,status,
                verified?axv::SipEvidence::VerifiedNativeApi:axv::SipEvidence::None,
                17,call};
            g_essential_actions=axv::essential_call_actions(snapshot,17,true);
            apply_faleweb_layout(window);
            const HWND dial=GetDlgItem(window,kCallButton);
            const HWND end=GetDlgItem(window,kHangupFuture);
            const HWND mute=GetDlgItem(window,kMuteFuture);
            const HWND hold=GetDlgItem(window,kHoldFuture);
            const HWND transfer=GetDlgItem(window,kTransferFuture);
            const bool incoming=expected==axv::EssentialCallView::Incoming;
            const bool idle=expected==axv::EssentialCallView::Idle;
            const bool secondary=expected==axv::EssentialCallView::Connected;
            passed=passed && g_essential_actions.view==expected &&
                visible(dial)==idle && visible(end)==!idle &&
                visible(mute)==secondary && visible(hold)==secondary &&
                visible(transfer)==secondary && in_client(g_number);
            passed=passed && essential_geometry_ok() && essential_labels_fit();
            if(visible(dial))passed=passed && in_client(dial);
            if(visible(end))passed=passed && in_client(end);
#ifdef AXV_ENABLE_NATIVE_SIP
            const HWND answer=GetDlgItem(window,kAnswerNative);
            passed=passed && visible(answer)==incoming;
            if(incoming){
                RECT a{},b{};GetWindowRect(answer,&a);GetWindowRect(end,&b);
                passed=passed && in_client(answer) && a.right<b.left &&
                    a.top==b.top && a.bottom==b.bottom;
            }
#else
            (void)incoming;
#endif
            if(secondary){
                RECT primary{},lower{};
                GetWindowRect(end,&primary);
                GetWindowRect(mute,&lower);
                passed=passed && in_client(mute) && in_client(hold) &&
                    in_client(transfer) && primary.bottom<lower.top;
            }
            if(!idle){
                wchar_t title[32]{};
                GetWindowTextW(end,title,32);
                passed=passed && std::wcscmp(title,incoming?L"Recusar":L"Encerrar")==0;
            }
            passed=passed && g_scroll_max==0 && g_scroll_offset==0 &&
                (GetWindowLongPtrW(window,GWL_STYLE)&WS_VSCROLL)==0;
        };
        const auto report_failed_stage=[&](const wchar_t* stage){
            if(!passed)
                report_design_failure(std::wstring(L"Falhou na etapa ") +
                    stage+L" com DPI "+std::to_wstring(g_dpi));
        };
        for(unsigned dpi:{96u,120u,144u,192u}){
            SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),0);
            RECT frame{};
            GetWindowRect(window,&frame);
            // On short Windows desktop work areas the entire canvas is
            // proportionally reduced instead of letting Windows clip it.
            const bool frame_ok=g_dpi>=96 && g_dpi<=dpi &&
                frame.right-frame.left==axv::dpi_pixels(kEssentialWidth,g_dpi) &&
                frame.bottom-frame.top==axv::dpi_pixels(kEssentialHeight,g_dpi);
            if(!frame_ok)report_design_failure(
                L"Janela divergente: "+std::to_wstring(frame.right-frame.left)+
                L"x"+std::to_wstring(frame.bottom-frame.top)+
                L" ao inves de "+std::to_wstring(axv::dpi_pixels(kEssentialWidth,g_dpi))+
                L"x"+std::to_wstring(axv::dpi_pixels(kEssentialHeight,g_dpi))+
                L", DPI layout "+std::to_wstring(g_dpi)+
                L", DPI sistema "+std::to_wstring(dpi));
            passed=passed && frame_ok;
            apply(axv::SipStatus::Registered,0,true,axv::EssentialCallView::Idle);
            report_failed_stage(L"registrado");
            apply(axv::SipStatus::Ringing,101,true,axv::EssentialCallView::Incoming);
            report_failed_stage(L"recebendo");
            apply(axv::SipStatus::Calling,101,true,axv::EssentialCallView::Outgoing);
            report_failed_stage(L"originando");
            apply(axv::SipStatus::InCall,101,true,axv::EssentialCallView::Connected);
            report_failed_stage(L"em_chamada");
            apply(axv::SipStatus::Ringing,101,false,axv::EssentialCallView::Idle);
            report_failed_stage(L"evento_nao_confiavel");
            apply(axv::SipStatus::Ringing,0,true,axv::EssentialCallView::Idle);
            report_failed_stage(L"token_zerado");
            // Monitor navigation must preserve the active incoming layout.
            apply(axv::SipStatus::Ringing,102,true,axv::EssentialCallView::Incoming);
            report_failed_stage(L"recebendo_apos_refresh");
            show_page(window,3);
            // The fourth page has an independent scroll-free viewport and
            // must not leave dialpad controls behind the monitor.
            passed=passed && visible(g_monitor_events) &&
                in_client(g_monitor_events) && !visible(g_number) &&
                !visible(GetDlgItem(window,kDigitStart)) &&
                !visible(g_footer_panel) && g_scroll_max==0 &&
                !(GetWindowLongPtrW(window,GWL_STYLE)&WS_VSCROLL);
            report_failed_stage(L"monitor");
            const bool original_dark=g_dark;
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kTheme,BN_CLICKED),0);
            passed=passed && g_dark!=original_dark &&
                in_client(g_monitor_events);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kTheme,BN_CLICKED),0);
            passed=passed && g_dark==original_dark;
            report_failed_stage(L"tema");
            show_page(window,0);
            passed=passed && essential_geometry_ok() &&
                essential_labels_fit() && visible(GetDlgItem(window,kHangupFuture));
            report_failed_stage(L"retorno_monitor");
#ifdef AXV_ENABLE_NATIVE_SIP
            passed=passed && visible(GetDlgItem(window,kAnswerNative));
#endif
        }
        if(!passed) {
            if(design_failure.empty())
                design_failure=L"Outro critério de aceitação do layout não passou.";
            FILE* report=nullptr;
            if(_wfopen_s(&report,L"AXV_DESIGN_FAILURE.txt",L"wt")==0 && report) {
                std::fwprintf(report,L"%ls\n",design_failure.c_str());
                std::fclose(report);
            }
        }
        DestroyWindow(window);
        return passed?0:97;
    }
    if(g_faleweb_smoke){
        bool passed=g_faleweb_mode && !g_scroll_max &&
            !((GetWindowLongPtrW(window,GWL_STYLE))&WS_VSCROLL) &&
            g_faleweb_prefs_button && (GetWindowLongPtrW(g_faleweb_prefs_button,GWL_STYLE)&WS_VISIBLE) &&
            g_faleweb_actions_button && (GetWindowLongPtrW(g_faleweb_actions_button,GWL_STYLE)&WS_VISIBLE) &&
            g_runtime_banner && (GetWindowLongPtrW(g_runtime_banner,GWL_STYLE)&WS_VISIBLE) &&
            g_number && (GetWindowLongPtrW(g_number,GWL_STYLE)&WS_VISIBLE) &&
            g_brand_label && (GetWindowLongPtrW(g_brand_label,GWL_STYLE)&WS_VISIBLE) &&
            (GetWindowLongPtrW(GetDlgItem(window,kTheme),GWL_STYLE)&WS_VISIBLE);
        RECT frame{},footer{},display{},number{},call{},brand{},theme{};
        GetWindowRect(g_brand_label,&brand);
        GetWindowRect(GetDlgItem(window,kTheme),&theme);
        GetWindowRect(window,&frame);
        GetWindowRect(g_footer_panel,&footer);
        GetWindowRect(g_runtime_banner,&display);
        GetWindowRect(g_number,&number);
        GetWindowRect(GetDlgItem(window,kCallButton),&call);
        passed=passed && frame.right-frame.left==axv::dpi_pixels(kFalewebWidth,g_dpi) &&
            frame.bottom-frame.top==axv::dpi_pixels(kFalewebHeight,g_dpi) &&
            brand.right<=theme.left && brand.bottom<=display.top &&
            theme.bottom<=display.top && display.bottom<number.top && number.bottom<call.top &&
            call.bottom<footer.top && footer.bottom<=frame.bottom;
        for(int index=0;index<12;++index){
            HWND key=GetDlgItem(window,kDigitStart+index);
            RECT bounds{};GetWindowRect(key,&bounds);
            passed=passed && key && (GetWindowLongPtrW(key,GWL_STYLE)&WS_VISIBLE) &&
                bounds.right-bounds.left==axv::dpi_pixels(72,g_dpi) &&
                bounds.bottom-bounds.top==axv::dpi_pixels(33,g_dpi) &&
                bounds.bottom<call.top;
            if(index%3!=2){
                RECT next{};
                GetWindowRect(GetDlgItem(window,kDigitStart+index+1),&next);
                passed=passed && bounds.right<next.left;
            }
            if(index<9){
                RECT next{};
                GetWindowRect(GetDlgItem(window,kDigitStart+index+3),&next);
                passed=passed && bounds.bottom<next.top;
            }
        }
        // Test the compact controls and fixed footer at 100%, 125% and 150%.
        // Both action rows must stay in the client content area at every DPI.
        for(unsigned scale:{96u,120u,144u}){
            SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(scale,scale),0);
            RECT safe_footer{},hangup{},mute{},call_box{},answer{},number_box{},brand_box{},theme_box{},display_box{};
            GetWindowRect(g_brand_label,&brand_box);
            GetWindowRect(GetDlgItem(window,kTheme),&theme_box);
            GetWindowRect(g_runtime_banner,&display_box);
            passed=passed && brand_box.right<=theme_box.left &&
                brand_box.bottom<=display_box.top && theme_box.bottom<=display_box.top;
            GetWindowRect(g_footer_panel,&safe_footer);
            GetWindowRect(GetDlgItem(window,kHangupFuture),&hangup);
            GetWindowRect(GetDlgItem(window,kMuteFuture),&mute);
            GetWindowRect(GetDlgItem(window,kCallButton),&call_box);
            GetWindowRect(g_number,&number_box);
            passed=passed && g_scroll_max==0 && g_scroll_offset==0 &&
                hangup.bottom<=safe_footer.top && mute.bottom<=safe_footer.top &&
                call_box.bottom<hangup.top && number_box.bottom<call_box.top &&
                (GetWindowLongPtrW(window,GWL_STYLE)&WS_VSCROLL)==0;
#ifdef AXV_ENABLE_NATIVE_SIP
            GetWindowRect(GetDlgItem(window,kAnswerNative),&answer);
            passed=passed && answer.bottom<hangup.top && answer.bottom<safe_footer.top;
#endif
            for(int key_index=0;key_index<12;++key_index){
                RECT rect{};
                GetWindowRect(GetDlgItem(window,kDigitStart+key_index),&rect);
                passed=passed && rect.bottom<call_box.top &&
                    rect.right-rect.left==axv::dpi_pixels(72,scale) &&
                    rect.bottom-rect.top==axv::dpi_pixels(33,scale);
            }
        }
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(96,96),0);
        const bool original_theme=g_dark;
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kTheme,BN_CLICKED),0);
        passed=passed && g_dark!=original_theme && g_scroll_max==0;
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kTheme,BN_CLICKED),0);
        passed=passed && g_dark==original_theme && g_scroll_max==0;
        for(int step=0;step<1000;++step){
            SendMessageW(window,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),0);
            SendMessageW(window,WM_VSCROLL,SB_BOTTOM,0);
            passed=passed && g_scroll_offset==0 && g_scroll_max==0;
        }
        DestroyWindow(window);
        return passed?0:93;
    }
    if (g_smoke_mode) {
        if(g_compact){g_compact=false;apply_compact_layout(window);}
        for(int pass=0;pass<2;++pass) {
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kShowAudio,BN_CLICKED),0);
            SendMessageW(window,WM_DEVICECHANGE,0x8000,0);
            SendMessageW(window,WM_DEVICECHANGE,0x8004,1);
            SendMessageW(window,WM_DEVICECHANGE,0x8001,1);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kRefreshHeadsetsButton,BN_CLICKED),0);
            SendMessageW(window,WM_TIMER,kProcessTimer,0);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kShowSettings,BN_CLICKED),0);
            SendMessageW(window,WM_DEVICECHANGE,0x7,0);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kAudioDevicesButton,BN_CLICKED),0);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kAudioDevicesButton,BN_CLICKED),0);
            SendMessageW(window,WM_TIMER,kProcessTimer,0);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
            SendMessageW(window,WM_DEVICECHANGE,0x18,1);
            SendMessageW(window,WM_TIMER,kProcessTimer,0);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
        }
        const auto audio=g_audio_worker.snapshot();
        const auto settings=g_settings_audio_worker.snapshot();
        if(g_audio_endpoint_watcher.active() || audio.busy || audio.pending || audio.ready || settings.busy || settings.pending || settings.ready ||
            g_audio_inventory_current || !GetDlgItem(window,kAudioDevicesButton)) {
            DestroyWindow(window); return 22;
        }
        show_page(window,0);
    }
#ifdef AXV_ENABLE_NATIVE_SIP
    if(command_line && std::wcscmp(command_line,L"--native-ui-smoke")==0) {
        if(g_compact){g_compact=false;apply_compact_layout(window);}
        bool ok=g_password && g_ca_file && g_remember_password &&
            (GetWindowLongPtrW(g_remember_password,GWL_STYLE)&BS_AUTOCHECKBOX)==BS_AUTOCHECKBOX &&
            GetDlgItem(window,kAnswerNative) &&
            GetDlgItem(window,kMuteFuture) && GetDlgItem(window,kHoldFuture) && GetDlgItem(window,kTransferFuture) &&
            !IsWindowEnabled(GetDlgItem(window,kTransferFuture)) &&
            !IsWindowEnabled(GetDlgItem(window,kMuteFuture)) && !IsWindowEnabled(GetDlgItem(window,kHoldFuture)) &&
            (GetWindowLongPtrW(g_password,GWL_STYLE)&ES_PASSWORD) &&
            !IsWindowEnabled(GetDlgItem(window,kCallButton));
        ok=ok && GetDlgItem(window,kDtmfButton) && !IsWindowEnabled(GetDlgItem(window,kDtmfButton));
        // No unsolicited incoming alert or system-tray message is generated
        // by a registered/disconnected UI lacking a verified incoming call.
        ok=ok && g_last_incoming_visual_session==0 &&
            g_last_incoming_visual_call==0 &&
            !axv::verified_incoming_call(native_engine().observation());
        const auto inactive_duration=[] {
            wchar_t text[80]{};
            GetWindowTextW(g_call_duration_label,text,80);
            return GetDlgItem(GetParent(g_call_duration_label),kCallDurationLabel)==g_call_duration_label &&
                   std::wstring_view(text)==L"DURACAO --:--:--";
        };
        const auto duration_layout_clear=[&] {
            RECT runtime{},duration{},ringtone{},destination{};
            GetWindowRect(g_runtime_banner,&runtime);
            GetWindowRect(g_call_duration_label,&duration);
            GetWindowRect(g_ringtone_status_label,&ringtone);
            GetWindowRect(g_number_label,&destination);
            bool dial_section_clear=true;
            for(const auto& child:g_layout) {
                if(child.logical.y!=220 || !(GetWindowLongPtrW(child.handle,GWL_STYLE)&WS_VISIBLE)) continue;
                RECT section{};
                GetWindowRect(child.handle,&section);
                dial_section_clear=duration.bottom<=section.top && ringtone.bottom<=section.top;
            }
            return (GetWindowLongPtrW(g_call_duration_label,GWL_STYLE)&WS_VISIBLE) &&
                   (GetWindowLongPtrW(g_ringtone_status_label,GWL_STYLE)&WS_VISIBLE) &&
                   runtime.bottom<=duration.top && duration.right<=ringtone.left &&
                   duration.top==ringtone.top && duration.bottom==ringtone.bottom &&
                   ringtone.right<=runtime.right &&
                   duration.bottom<=destination.top && ringtone.bottom<=destination.top &&
                   dial_section_clear;
        };
        RECT recents_bounds{},number_label_bounds{};
        GetWindowRect(GetDlgItem(window,kRecentDestinationsButton),&recents_bounds);
        GetWindowRect(g_number_label,&number_label_bounds);
        ok=ok && GetDlgItem(window,kRecentDestinationsButton) &&
            !IsWindowEnabled(GetDlgItem(window,kRecentDestinationsButton)) &&
            recents_bounds.bottom<=number_label_bounds.top;
        RECT contact_bounds{};
        GetWindowRect(GetDlgItem(window,kContactsButton),&contact_bounds);
        ok=ok && GetDlgItem(window,kContactsButton) && IsWindowEnabled(GetDlgItem(window,kContactsButton)) &&
            contact_bounds.right<=recents_bounds.left && contact_bounds.bottom<=number_label_bounds.top;
        if(!ok){DestroyWindow(window);return 31;} // Native baseline geometry.
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kContactsButton,BN_CLICKED),0);
        HWND contacts=FindWindowW(L"AXVContactBookV1",nullptr);
        ok=ok && contacts && IsWindowEnabled(window) && GetDlgItem(contacts,101) && GetDlgItem(contacts,103) &&
            GetDlgItem(contacts,104) && GetDlgItem(contacts,105) && GetDlgItem(contacts,113);
        MSG contact_key{};contact_key.hwnd=GetDlgItem(contacts,101);contact_key.message=WM_KEYDOWN;contact_key.wParam='A';
        ok=ok && axv::contacts_owns_message(contact_key) && !IsWindowEnabled(GetDlgItem(window,kCallButton));
        show_page(window,1);
        const auto before_contact=native_engine().observation();
        ok=ok && axv::use_contact_destination(g_number,{99,L"Contato",L"1002",false}) && g_page==0;
        const auto after_contact=native_engine().observation();
        ok=ok && before_contact.session==after_contact.session && before_contact.call_id==after_contact.call_id &&
            before_contact.sip==after_contact.sip && !IsWindowEnabled(GetDlgItem(window,kCallButton));
        SetWindowTextW(g_number,L"");
        axv::close_contact_book();
        ok=ok && !FindWindowW(L"AXVContactBookV1",nullptr);
        // Empty/synthetic commands never open a popup or create a dial request.
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kRecentDestinationsButton,BN_CLICKED),0);
        ok=ok && g_recent_destinations.size()==0 && !IsWindowEnabled(GetDlgItem(window,kCallButton));
        if(!ok){DestroyWindow(window);return 32;} // Contacts and shared SIP state.
        const auto before_sound_mode=native_engine().observation();
        const auto same_sound_observation=[&] {
            const auto after=native_engine().observation();
            return after.process==before_sound_mode.process && after.sip==before_sound_mode.sip &&
                after.evidence==before_sound_mode.evidence && after.session==before_sound_mode.session &&
                after.call_id==before_sound_mode.call_id && after.last_call_id==before_sound_mode.last_call_id;
        };
        const auto sound_badge=[] {
            wchar_t ringtone[32]{};
            wchar_t duration[80]{};
            GetWindowTextW(g_ringtone_status_label,ringtone,32);
            GetWindowTextW(g_call_duration_label,duration,80);
            return std::wstring_view(ringtone)==(g_incoming_ringtone_muted?L"TOQUE MUDO":L"") &&
                   std::wstring_view(duration)==L"DURACAO --:--:--";
        };
        HWND sound_button=GetDlgItem(window,kIncomingRingtoneMuteButton);
        const auto sound_button_matches=[&] {
            wchar_t text[40]{};GetWindowTextW(sound_button,text,40);
            return std::wstring_view(text)==(g_incoming_ringtone_muted?L"TOQUE MUDO":L"TOQUE NORMAL");
        };
        RECT sound_bounds{},dtmf_pair_bounds{},sound_feedback_bounds{};
        GetWindowRect(sound_button,&sound_bounds);GetWindowRect(GetDlgItem(window,kDtmfButton),&dtmf_pair_bounds);
        GetWindowRect(g_status,&sound_feedback_bounds);
        ok=ok && sound_button && IsWindowEnabled(sound_button) && sound_button_matches() && !g_incoming_ringtone_muted &&
            dtmf_pair_bounds.right<=sound_bounds.left && dtmf_pair_bounds.top==sound_bounds.top &&
            (sound_bounds.bottom<=sound_feedback_bounds.top ||
             sound_bounds.top>=sound_feedback_bounds.top);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kIncomingRingtoneMuteButton,BN_CLICKED),0);
        ok=ok && g_incoming_ringtone_muted && sound_button_matches() && sound_badge() && same_sound_observation() &&
            !g_ring_player.snapshot().request.active() && !IsWindowEnabled(GetDlgItem(window,kCallButton));
        if(!ok){DestroyWindow(window);return 35;} // Mute-button first toggle.
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
        ok=ok && g_compact && sound_badge() && duration_layout_clear() &&
            (GetWindowLongPtrW(g_call_duration_label,GWL_STYLE)&WS_VISIBLE) &&
            (GetWindowLongPtrW(g_ringtone_status_label,GWL_STYLE)&WS_VISIBLE) &&
            !(GetWindowLongPtrW(sound_button,GWL_STYLE)&WS_VISIBLE);
        // Accelerator-style dispatch works even while the button is hidden.
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kIncomingRingtoneMuteButton,1),0);
        ok=ok && !g_incoming_ringtone_muted && sound_button_matches() && sound_badge() && same_sound_observation();
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
        ok=ok && inactive_duration() && duration_layout_clear();
        // Regression: both labels stay distinct after DPI changes and compact mode.
        // This never performs a SIP call or writes user preferences.
        for(unsigned scale : {96u,120u,144u,192u}) {
            SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(scale,scale),0);
            ok=ok && duration_layout_clear() && sound_badge() && inactive_duration();
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
            ok=ok && g_compact && duration_layout_clear();
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
            ok=ok && !g_compact && duration_layout_clear();
        }
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(96,96),0);
        RECT dtmf_bounds{},feedback_bounds{};
        GetWindowRect(GetDlgItem(window,kDtmfButton),&dtmf_bounds);GetWindowRect(g_status,&feedback_bounds);
        ok=ok && (dtmf_bounds.bottom<=feedback_bounds.top || dtmf_bounds.top>=feedback_bounds.top);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
        ok=ok && g_compact && inactive_duration() && duration_layout_clear();
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCompact,BN_CLICKED),0);
        GetWindowRect(GetDlgItem(window,kDtmfButton),&dtmf_bounds);GetWindowRect(g_status,&feedback_bounds);
        ok=ok && !g_compact && (dtmf_bounds.bottom<=feedback_bounds.top || dtmf_bounds.top>=feedback_bounds.top);
        if(!ok){DestroyWindow(window);return 36;} // Compact / duration.
        SetWindowPos(window,nullptr,0,0,axv::dpi_pixels(kWindowWidth,g_dpi),axv::dpi_pixels(550,g_dpi),SWP_NOMOVE|SWP_NOZORDER);
        SendMessageW(window,WM_VSCROLL,MAKEWPARAM(SB_BOTTOM,0),0);
        GetWindowRect(GetDlgItem(window,kDtmfButton),&dtmf_bounds);GetWindowRect(g_status,&feedback_bounds);
        ok=ok && g_scroll_offset>0 && (dtmf_bounds.bottom<=feedback_bounds.top || dtmf_bounds.top>=feedback_bounds.top);
        SendMessageW(window,WM_VSCROLL,MAKEWPARAM(SB_TOP,0),0);
        GetWindowRect(GetDlgItem(window,kDtmfButton),&dtmf_bounds);GetWindowRect(g_status,&feedback_bounds);
        ok=ok && g_scroll_offset==0 && (dtmf_bounds.bottom<=feedback_bounds.top || dtmf_bounds.top>=feedback_bounds.top);
        if(!ok){DestroyWindow(window);return 37;} // Scroll top/bottom.
        // Exercise the button's real focus notification while remaining hidden.
        const HWND clear_button=GetDlgItem(window,kClear);
        SendMessageW(clear_button,WM_SETFOCUS,0,0);
        ok=ok && g_scroll_offset>0 && (GetWindowLongPtrW(g_tls,GWL_STYLE)&BS_NOTIFY);
        SendMessageW(clear_button,WM_KILLFOCUS,0,0);
        SendMessageW(window,WM_VSCROLL,MAKEWPARAM(SB_TOP,0),0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kDtmfButton,BN_CLICKED),0);
        ok=ok && !g_dtmf_mode.armed();
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCallButton,BN_CLICKED),0);
        ok=ok && native_engine().status()==axv::SipStatus::Disconnected;
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kMuteFuture,BN_CLICKED),0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kHoldFuture,BN_CLICKED),0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kTransferFuture,BN_CLICKED),0);
        ok=ok && !native_engine().media_state().available && native_engine().status()==axv::SipStatus::Disconnected;
        if(!ok){DestroyWindow(window);return 38;} // Focus / disabled DTMF.
        refresh_runtime_banner(g_baresip_process.poll());
        ok=ok && inactive_duration() && duration_layout_clear();
        if(!ok){DestroyWindow(window);return 33;} // Mute and scrolling layout.
        std::vector<axv::AudioEndpoint> endpoints{{L"present",L"Microphone",true,false}};
        std::wstring missing=L"missing";
        const auto fixed=fill_audio_combo(g_input_audio_combo,endpoints,missing,false,true,true);
        ok=ok && !fixed.available() && missing==L"missing" && !IsWindowEnabled(g_input_audio_combo);
        std::wstring selected=L"present";
        const auto editable=fill_audio_combo(g_input_audio_combo,endpoints,selected,false,false,true);
        ok=ok && editable.available() && IsWindowEnabled(g_input_audio_combo);
        std::wstring windows_default;
        fill_audio_combo(g_input_audio_combo,endpoints,windows_default,false,false,true);
        ok=ok && windows_default.empty() && SendMessageW(g_input_audio_combo,CB_GETCURSEL,0,0)==0;
        g_audio_inputs=endpoints;g_selected_input_id=L"present";
        fill_audio_combo(g_input_audio_combo,endpoints,g_selected_input_id,false,false,true);
        SendMessageW(g_input_audio_combo,CB_SETCURSEL,0,0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kInputAudioCombo,CBN_SELCHANGE),reinterpret_cast<LPARAM>(g_input_audio_combo));
        ok=ok && g_selected_input_id.empty();
        std::vector<axv::AudioEndpoint> ring_outputs{{L"voice",L"Voice",true,false},{L"multimedia",L"Ring",false,true}};
        std::wstring ring_default;
        const auto ring=fill_audio_combo(g_ringtone_audio_combo,ring_outputs,ring_default,true,false,true);
        ok=ok && ring.index==1 && ring_default.empty() && SendMessageW(g_ringtone_audio_combo,CB_GETCURSEL,0,0)==0;
        std::wstring ring_missing=L"missing";
        const auto no_fallback=fill_audio_combo(g_ringtone_audio_combo,ring_outputs,ring_missing,true,false,true);
        ok=ok && !no_fallback.available() && ring_missing==L"missing";
        g_audio_outputs=ring_outputs;g_selected_ringtone_id=L"multimedia";
        fill_audio_combo(g_ringtone_audio_combo,ring_outputs,g_selected_ringtone_id,true,false,true);
        SendMessageW(g_ringtone_audio_combo,CB_SETCURSEL,0,0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kRingtoneAudioCombo,CBN_SELCHANGE),reinterpret_cast<LPARAM>(g_ringtone_audio_combo));
        ok=ok && g_selected_ringtone_id.empty() && !can_preview_selected_ringtone() && !g_ring_player.snapshot().request.active();
        // A failed inventory must retain selections without timer re-enabling them.
        const axv::AudioPanelSelection retained{g_selected_input_id,g_selected_output_id,g_selected_ringtone_id};
        axv::AudioInventory inventory_error;inventory_error.error=axv::AudioInventoryError::EndpointQueryFailed;
        apply_audio_inventory(inventory_error);
        refresh_runtime_banner(g_baresip_process.poll());
        ok=ok && g_selected_input_id==retained.input_id && g_selected_output_id==retained.output_id &&
            g_selected_ringtone_id==retained.ringtone_id && !g_audio_inventory_current &&
            !IsWindowEnabled(g_input_audio_combo) && !IsWindowEnabled(g_output_audio_combo) &&
            !IsWindowEnabled(g_ringtone_audio_combo);
        if(!ok){DestroyWindow(window);return 34;} // Audio selection invariants.
        // A missing CA must reject TLS before any registration starts.
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kReadinessButton,BN_CLICKED),0);
        ok=ok && g_readiness_requested_smoke && !IsWindowEnabled(GetDlgItem(window,kCallButton));
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCopyStatusButton,BN_CLICKED),0);
        ok=ok && g_copy_requested_smoke && GetDlgItem(window,kCopyStatusButton);
        g_copy_requested_smoke=false;
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCopyStatusButton,1),0);
        ok=ok && g_copy_requested_smoke && native_engine().status()==axv::SipStatus::Disconnected;
        axv::SipAccount a{L"127.0.0.1",L"1001",5061,true};
        ok=ok && !native_engine().configure_authenticated(a,L"","fixture-only");
        DestroyWindow(window); return ok?0:21;
    }
#endif
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
        // Smoke-check the owner-drawn action controls (no SIP calls).
        for (const int action_id : {kCallButton,kStartBaresipButton,kStopBaresipButton}) {
            const HWND action=GetDlgItem(window,action_id);
            ok=ok && action &&
                ((GetWindowLongPtrW(action,GWL_STYLE)&BS_TYPEMASK)==BS_OWNERDRAW);
        }
        // Verify five choices are selectable, and custom MP3 requires a path.
        ok=ok && g_ringtone_preset_combo &&
            SendMessageW(g_ringtone_preset_combo,CB_GETCOUNT,0,0)==6 &&
            GetDlgItem(window,kRingtoneChooseMp3);
        const int original_preset=g_ringtone_preset.load();
        std::wstring original_mp3;
        {std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);
         original_mp3=g_ringtone_custom_mp3;g_ringtone_custom_mp3.clear();}
        for(int tone=0;tone<5;++tone){
            SendMessageW(g_ringtone_preset_combo,CB_SETCURSEL,tone,0);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kRingtonePresetCombo,CBN_SELCHANGE),
                         reinterpret_cast<LPARAM>(g_ringtone_preset_combo));
            ok=ok && g_ringtone_preset.load()==tone;
        }
        SendMessageW(g_ringtone_preset_combo,CB_SETCURSEL,5,0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kRingtonePresetCombo,CBN_SELCHANGE),
                     reinterpret_cast<LPARAM>(g_ringtone_preset_combo));
        ok=ok && g_ringtone_preset.load()==4 &&
            SendMessageW(g_ringtone_preset_combo,CB_GETCURSEL,0,0)==4;
        {std::lock_guard<std::mutex> lock(g_ringtone_mp3_mutex);
         g_ringtone_custom_mp3=std::move(original_mp3);}
        g_ringtone_preset.store(original_preset);
        SendMessageW(g_ringtone_preset_combo,CB_SETCURSEL,original_preset,0);
        for (int i = 0; i != 12; ++i) {
            const HWND key = GetDlgItem(window, kDigitStart + i);
            RECT bounds{};
            if (key) GetWindowRect(key, &bounds);
            ok = ok && key &&
                 bounds.right-bounds.left == axv::dpi_pixels(114,g_dpi) &&
                 bounds.bottom-bounds.top == axv::dpi_pixels(44,g_dpi);
        }
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
        // 1,000 real Win32 key command cycles. Every cycle checks the
        // inserted digit, keypad geometry and isolation from neighboring keys.
        // No SIP network request is made by this smoke test.
        bool thousand_key_checks = true;
        for (int trial=0; trial<1000 && thousand_key_checks; ++trial) {
            const int key_index = trial % 12;
            const HWND key = GetDlgItem(window,kDigitStart+key_index);
            SetWindowTextW(g_number,L"1234");
            SendMessageW(g_number,EM_SETSEL,2,2);
            SendMessageW(window,WM_COMMAND,MAKEWPARAM(kDigitStart+key_index,BN_CLICKED),
                         reinterpret_cast<LPARAM>(key));
            wchar_t result[32]{};
            GetWindowTextW(g_number,result,32);
            const auto digit=axv::dialpad_keys()[static_cast<std::size_t>(key_index)];
            const std::wstring expected=L"12"+std::wstring(1,digit)+L"34";
            RECT bounds{};
            GetWindowRect(key,&bounds);
            thousand_key_checks = key && std::wstring_view(result)==expected &&
                bounds.right>bounds.left && bounds.bottom>bounds.top;
            if (thousand_key_checks) {
                // All 12 buttons must stay non-overlapping after key presses.
                for (int sibling=0;sibling<12;++sibling) {
                    if(sibling==key_index)continue;
                    RECT other{};
                    GetWindowRect(GetDlgItem(window,kDigitStart+sibling),&other);
                    const bool intersects = bounds.left<other.right &&
                        other.left<bounds.right && bounds.top<other.bottom &&
                        other.top<bounds.bottom;
                    if(intersects){thousand_key_checks=false;break;}
                }
            }
        }
        ok=ok && thousand_key_checks;
        // 3x4 keypad alignment, exact 4-logical-pixel gutters and a
        // separated action row, checked through real Win32 rectangles.
        bool grid_geometry_ok=true;
        for(int key_index=0;key_index<12;++key_index){
            RECT key{};GetWindowRect(GetDlgItem(window,kDigitStart+key_index),&key);
            const int width=axv::dpi_pixels(114,g_dpi);
            const int height=axv::dpi_pixels(44,g_dpi);
            grid_geometry_ok=grid_geometry_ok &&
                key.right-key.left==width && key.bottom-key.top==height;
            if(key_index%3!=2){
                RECT next{};GetWindowRect(GetDlgItem(window,kDigitStart+key_index+1),&next);
                grid_geometry_ok=grid_geometry_ok &&
                    next.left-key.right==axv::dpi_pixels(118,g_dpi)-width;
            }
            if(key_index<9){
                RECT next{};GetWindowRect(GetDlgItem(window,kDigitStart+key_index+3),&next);
                grid_geometry_ok=grid_geometry_ok &&
                    next.top-key.bottom==axv::dpi_pixels(48,g_dpi)-height;
            }
        }
        RECT last_digit{},action_row{};
        GetWindowRect(GetDlgItem(window,kDigitStart+11),&last_digit);
        GetWindowRect(GetDlgItem(window,kBackspace),&action_row);
        grid_geometry_ok=grid_geometry_ok && last_digit.bottom<action_row.top;
        ok=ok && grid_geometry_ok;
        SetWindowTextW(g_number, L"");
        GetWindowTextW(g_status, feedback_message, 160);
        const bool empty_feedback_ok = !g_destination_invalid &&
            std::wstring_view(feedback_message).find(L"Digite") != std::wstring_view::npos;
        ok = ok && invalid_feedback_ok && valid_feedback_ok && empty_feedback_ok && direct_command_denied;
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kCompact, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window, kCompact)));
        RECT bounds = {};
        GetWindowRect(window, &bounds);
        RECT compact_input{},compact_footer{};
        GetWindowRect(g_number,&compact_input);
        GetWindowRect(g_footer_panel,&compact_footer);
        const bool compact_separation=compact_input.bottom<=compact_footer.top;
        const bool compact_ok = g_compact && compact_separation &&
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
        const bool audio_ok = g_page == 2 &&
            (GetWindowLongW(g_input_audio_combo, GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(g_output_audio_combo, GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(g_ringtone_audio_combo, GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(GetDlgItem(window, kTestRingtoneButton), GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(GetDlgItem(window, kTestConversationButton), GWL_STYLE) & WS_VISIBLE) &&
            (GetWindowLongW(GetDlgItem(window, kTestMicrophoneButton), GWL_STYLE) & WS_VISIBLE) &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton)) &&
            !IsWindowEnabled(g_input_audio_combo) &&
            !IsWindowEnabled(GetDlgItem(window, kTestRingtoneButton)) &&
            !IsWindowEnabled(GetDlgItem(window, kTestConversationButton)) &&
            !IsWindowEnabled(GetDlgItem(window, kTestMicrophoneButton)) &&
            !IsWindowEnabled(g_ringtone_audio_combo); // No hardware inspected in smoke.
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(kReadinessButton, 1), 0);
        const bool readiness_ok = g_readiness_requested_smoke &&
            GetDlgItem(window, kReadinessButton) &&
            (GetWindowLongW(GetDlgItem(window, kReadinessButton), GWL_STYLE) & WS_VISIBLE) &&
            !IsWindowEnabled(GetDlgItem(window, kCallButton));
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCopyStatusButton,BN_CLICKED),0);
        const bool copy_button_ok=g_copy_requested_smoke && GetDlgItem(window,kCopyStatusButton) &&
            (GetWindowLongW(GetDlgItem(window,kCopyStatusButton),GWL_STYLE)&WS_VISIBLE);
        g_copy_requested_smoke=false;
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kCopyStatusButton,1),0);
        ok=ok && copy_button_ok && g_copy_requested_smoke && !IsWindowEnabled(GetDlgItem(window,kCallButton));
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
        RECT mini_number{},mini_footer{};
        GetWindowRect(g_number,&mini_number);
        GetWindowRect(g_footer_panel,&mini_footer);
        const bool compact_150_ok=g_compact &&
            mini_number.bottom<=mini_footer.top &&
            bounds.bottom-bounds.top==axv::dpi_pixels(kCompactHeight,144);
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(192,192),0);
        GetWindowRect(window,&bounds);
        MONITORINFO smoke_monitor{};
        smoke_monitor.cbSize = sizeof(smoke_monitor);
        const HMONITOR smoke_display = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        const int requested_mini_height = axv::dpi_pixels(kCompactHeight,192);
        int expected_mini_height = requested_mini_height;
        if (smoke_display && GetMonitorInfoW(smoke_display, &smoke_monitor)) {
            const int work_height = smoke_monitor.rcWork.bottom-smoke_monitor.rcWork.top;
            if (work_height > 0)
                expected_mini_height = (std::min)(requested_mini_height, work_height);
        }
        const bool compact_200_ok=g_dpi==192 && g_compact &&
            bounds.bottom-bounds.top==expected_mini_height;
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(kShowSettings,BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(window,kShowSettings)));
        const bool full_200_ok=!g_compact && g_page==1 &&
            !IsWindowEnabled(GetDlgItem(window,kCallButton));
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(96,96),0);
        ok=ok && dpi_150_ok && compact_150_ok && compact_200_ok &&
           full_200_ok && g_dpi==96 && !IsWindowEnabled(GetDlgItem(window,kCallButton));
        const auto footer_clear=[](HWND window){
            RECT panel{},status{},client{};
            GetWindowRect(g_footer_panel,&panel);
            GetWindowRect(g_status,&status);
            GetClientRect(window,&client);
            POINT origin{0,client.bottom};
            ClientToScreen(window,&origin);
            return panel.bottom==origin.y &&
                   status.left>=panel.left && status.right<=panel.right &&
                   status.top>=panel.top && status.bottom<=panel.bottom &&
                   (GetWindowLongPtrW(g_status,GWL_STYLE)&SS_TYPEMASK)==SS_LEFT;
        };
        // Exercise 100%, 125%, 150%, and 200% DPI, including a client
        // smaller than the footer. Both opaque panel and text must stay inside.
        for(unsigned scale : {96u,120u,144u,192u}) {
            SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(scale,scale),0);
            ok=ok && footer_clear(window);
            RECT original{};
            GetWindowRect(window,&original);
            SetWindowPos(window,nullptr,0,0,
                         original.right-original.left,
                         axv::dpi_pixels(70,g_dpi),
                         SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
            ok=ok && footer_clear(window);
            SetWindowPos(window,nullptr,0,0,
                         original.right-original.left,
                         original.bottom-original.top,
                         SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
            ok=ok && footer_clear(window);
        }
        SendMessageW(window,WM_DPICHANGED,MAKEWPARAM(96,96),0);
        ok=ok && footer_clear(window);
        const bool pre_scroll_ok = ok;
        // Verify scrolling on an artificially short viewport without
        // changing the machine's display settings or making SIP calls.
        RECT original_window{};
        GetWindowRect(window, &original_window);
        SetWindowPos(window, nullptr, 0, 0,
                     original_window.right-original_window.left,
                     axv::dpi_pixels(430,g_dpi),
                     SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        // Reproduce 1,000 rapid scrollbar-drag notifications. They must
        // coalesce into one pending GUI message; call control remains blocked.
        bool async_scroll_ok=true;
        for(int drag=0;drag<1000;++drag) {
            g_pending_scroll_offset=g_scroll_max*(drag%101)/100;
            // Drive the exact posted-message path without moving the cursor.
            SendMessageW(window,WM_VSCROLL,MAKEWPARAM(SB_THUMBTRACK,0),0);
            async_scroll_ok=async_scroll_ok && g_scroll_message_queued;
        }
        g_pending_scroll_offset=g_scroll_max;
        SendMessageW(window,kAsyncScrollMessage,0,0);
        async_scroll_ok=async_scroll_ok && !g_scroll_message_queued &&
            g_scroll_offset==g_scroll_max &&
            !IsWindowEnabled(GetDlgItem(window,kCallButton));
        SendMessageW(window,WM_VSCROLL,SB_TOP,0);
        async_scroll_ok=async_scroll_ok && g_scroll_offset==0;
        ok=ok && async_scroll_ok;
        ok=ok && footer_clear(window);
        const bool scroll_available = g_scroll_max > 0;
        SendMessageW(window, WM_VSCROLL, SB_PAGEDOWN, 0);
        const bool scroll_moved = g_scroll_offset > 0 &&
            !IsWindowEnabled(GetDlgItem(window,kCallButton));
        SendMessageW(window, WM_VSCROLL, SB_TOP, 0);
        const bool scroll_returned = g_scroll_offset == 0;
        const HWND account_button = GetDlgItem(window, kValidateAccount);
        SendMessageW(account_button, WM_SETFOCUS, 0, 0);
        const bool button_focus_scrolled = g_scroll_offset > 0 &&
            (GetWindowLongPtrW(g_tls, GWL_STYLE) & BS_NOTIFY);
        SendMessageW(account_button, WM_KILLFOCUS, 0, 0);
        SendMessageW(window, WM_VSCROLL, SB_TOP, 0);
        SetWindowPos(window, nullptr, 0, 0,
                     original_window.right-original_window.left,
                     original_window.bottom-original_window.top,
                     SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        ok = ok && scroll_available && scroll_moved && scroll_returned && button_focus_scrolled;
        if (!ok) {
            std::fprintf(stderr, "UI_VIEWPORT_SMOKE pre=%d scroll=%d moved=%d top=%d max=%d off=%d dpi150=%d mini150=%d mini200=%d full200=%d\n",
                         pre_scroll_ok ? 1 : 0, scroll_available ? 1 : 0,
                         scroll_moved ? 1 : 0, scroll_returned ? 1 : 0,
                         g_scroll_max, g_scroll_offset,
                         dpi_150_ok ? 1 : 0, compact_150_ok ? 1 : 0,
                         compact_200_ok ? 1 : 0, full_200_ok ? 1 : 0);
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
    ShowWindow(window, show);
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
#ifdef AXV_ENABLE_NATIVE_SIP
        {FVIRTKEY | FCONTROL, L'J', kContactsButton},
        {FVIRTKEY | FCONTROL, L'H', kRecentDestinationsButton},
        {FVIRTKEY | FCONTROL | FSHIFT, L'S', kIncomingRingtoneMuteButton},
#endif
        {FVIRTKEY, VK_ESCAPE, kShowDial},
        {FVIRTKEY, VK_F1, kHelpButton},
        {FVIRTKEY, VK_F2, kReadinessButton},
        {FVIRTKEY | FCONTROL | FSHIFT, L'D', kCopyStatusButton}
    };
    HACCEL accelerator = CreateAcceleratorTableW(
        const_cast<ACCEL*>(shortcuts), static_cast<int>(sizeof(shortcuts)/sizeof(shortcuts[0])));
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
#ifdef AXV_ENABLE_NATIVE_SIP
        const bool contact_message=axv::contacts_owns_message(msg);
        if(contact_message && axv::translate_contacts_message(msg))continue;
        if (!contact_message && accelerator && TranslateAcceleratorW(window, accelerator, &msg)) continue;
        if (!contact_message && IsDialogMessageW(window, &msg)) continue;
#else
        if (accelerator && TranslateAcceleratorW(window, accelerator, &msg)) continue;
        if (IsDialogMessageW(window, &msg)) continue;
#endif

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (accelerator) DestroyAcceleratorTable(accelerator);
    return static_cast<int>(msg.wParam);
}
