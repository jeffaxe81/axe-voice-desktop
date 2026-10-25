#include "../src/audio_test_tone_win32.h"
#include <atomic>
int main(){
 std::atomic<bool> cancel{true};
 if(axv::play_windows_tone_on_endpoint(L"",&cancel,true)!=axv::TonePreviewState::Cancelled)return 1;
 if(axv::play_windows_tone_on_endpoint(L"not-a-device",&cancel)!=axv::TonePreviewState::Cancelled)return 2;
 if(axv::play_windows_tone_on_endpoint(L"")!=axv::TonePreviewState::MissingDevice)return 3;
 if(axv::play_windows_tone_on_endpoint(std::wstring(L"a\0b",3))!=axv::TonePreviewState::MissingDevice)return 4;
 axv::TonePreviewPlayer preview;
 if(preview.start(L""))return 5;
 preview.request_stop();
 return 0; // Every executed path above rejects before COM/audio initialization.
}
