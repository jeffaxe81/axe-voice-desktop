#include "../src/native_audio_routes.h"
#include "../src/native_account_config.h"
#include "../src/native_audio_command_guard.h"
#include <cstdlib>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
int main(){
 axv::NativeAudioRoute selected{L"capture-id",L"playback-id"};
 CHECK(axv::valid_native_audio_route(selected));
 for(const auto& id:{L"bad\nmodule x",L"bad,id",L"bad id",L"bad#id",L"bad\"id",L"bad'id",L"bad\\id",L"DeFaUlT",L"default",L"\u00e9"})
  CHECK(!axv::valid_native_audio_route({id,L""}));
 CHECK(!axv::valid_native_audio_route({std::wstring(128,L'a'),L""}));
 axv::AudioInventory inventory;
 inventory.input_devices={{L"capture-id",L"Microphone",true,false}};
 inventory.output_devices={{L"playback-id",L"Headset",true,false}};
 inventory.has_default_communications_input=true;inventory.has_default_communications_output=true;
 CHECK(axv::native_audio_route_status(selected,inventory)==axv::NativeAudioRouteStatus::Ready);
 CHECK(axv::native_audio_route_status({},inventory)==axv::NativeAudioRouteStatus::Ready);
 const auto defaults=axv::fixed_native_audio_selection(inventory.input_devices,L"");
 CHECK(defaults.available() && defaults.selected_id.empty());
 CHECK(axv::native_audio_combo_index(inventory.input_devices,L"")==0);
 CHECK(axv::native_audio_combo_index(inventory.input_devices,L"capture-id")==1);
 CHECK(axv::native_audio_combo_index(inventory.input_devices,L"missing")==-1);
 auto replacement_defaults=inventory.input_devices;replacement_defaults[0].id=L"new-default";
 CHECK(axv::fixed_native_audio_selection(replacement_defaults,L"").selected_id.empty());
 CHECK(axv::native_audio_route_status({L"missing",L"playback-id"},inventory)==axv::NativeAudioRouteStatus::MissingCapture);
 CHECK(axv::native_audio_route_status({L"capture-id",L"missing"},inventory)==axv::NativeAudioRouteStatus::MissingPlayback);
 CHECK(axv::native_audio_route_status({L"playback-id",L"capture-id"},inventory)!=axv::NativeAudioRouteStatus::Ready);
 CHECK(axv::fixed_native_audio_selection(inventory.input_devices,L"missing").selected_id==L"missing");
 CHECK(!axv::fixed_native_audio_selection(inventory.input_devices,L"missing").available());
 inventory.input_devices[0].default_communications=false;
 CHECK(!axv::fixed_native_audio_selection(inventory.input_devices,L"").available());
 inventory.input_devices[0].default_communications=true;
 axv::SipRuntimeObservation ringing{axv::RuntimeProcess::Running,axv::SipStatus::Ringing,axv::SipEvidence::VerifiedNativeApi,1,1,1};
 axv::NativeCallCommand answer{axv::NativeCallAction::Answer,1,1,{}};
 axv::NativeCommandAdmission admission;
 auto decision=axv::admit_native_audio_command(admission,answer,ringing,selected,{});
 CHECK(!decision.allowed && decision.audio==axv::NativeAudioRouteStatus::MissingCapture);
 decision=axv::admit_native_audio_command(admission,answer,ringing,selected,inventory);
 CHECK(decision.allowed); // Reconnecting must permit the still-current answer.
 CHECK(!axv::admit_native_audio_command(admission,answer,ringing,selected,inventory).allowed);
 axv::NativeCallCommand hangup{axv::NativeCallAction::Hangup,1,1,{}};
 CHECK(axv::admit_native_audio_command(admission,hangup,ringing,selected,{}).allowed);
 ringing.evidence=axv::SipEvidence::None;
 CHECK(!axv::admit_native_audio_command(admission,answer,ringing,selected,inventory).allowed);
 inventory.error=axv::AudioInventoryError::EndpointQueryFailed;
 CHECK(axv::native_audio_route_status(selected,inventory)==axv::NativeAudioRouteStatus::InventoryFailed);
 axv::SipAccount account{L"pbx.example",L"1001",5060,false};
 const auto cfg=axv::native_account_config(account,L"",selected);
 CHECK(cfg.valid && cfg.config.find("audio_source wasapi,capture-id\n")!=std::string::npos);
 CHECK(cfg.config.find("audio_player wasapi,playback-id\n")!=std::string::npos);
 CHECK(!axv::native_account_config(account,L"",{L"bad\nvalue",L""}).valid);
}
