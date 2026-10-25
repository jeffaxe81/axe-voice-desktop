#pragma once
#include "audio_selection_feedback.h"
#include <string>
namespace axv {
struct NativeAudioRoute { std::wstring capture_id,playback_id; };
enum class NativeAudioRouteStatus {Ready,InvalidSelection,InventoryFailed,MissingCapture,MissingPlayback};
inline bool valid_native_audio_id(const std::wstring& id){
 // Baresip4.12's device buffer is128 bytes. IDs are opaque, not display names.
 if(id.size()>127)return false;
 auto reserved=id;
 for(auto& c:reserved)if(c>=L'A' && c<=L'Z')c+=L'a'-L'A';
 if(reserved==L"default")return false;
 for(wchar_t c:id)if(c<=32 || c>=127 || c==L',' || c==L'"' || c==L'#' || c==L';' || c==L'\\' || c==L'\'')return false;
 return true;
}
inline bool valid_native_audio_route(const NativeAudioRoute& route){
 return valid_native_audio_id(route.capture_id)&&valid_native_audio_id(route.playback_id);
}
inline AudioSelectionRefresh fixed_native_audio_selection(const std::vector<AudioEndpoint>& devices,const std::wstring& id){
 for(std::size_t i=0;i<devices.size();++i)
  if(id.empty()?devices[i].default_communications:devices[i].id==id)
   return {static_cast<int>(i),id,false};
 return {-1,id,false}; // Never replace a session-bound endpoint silently.
}
inline NativeAudioRouteStatus native_audio_route_status(const NativeAudioRoute& route,const AudioInventory& inventory){
 if(!valid_native_audio_route(route))return NativeAudioRouteStatus::InvalidSelection;
 if(inventory.error!=AudioInventoryError::None)return NativeAudioRouteStatus::InventoryFailed;
 if(!fixed_native_audio_selection(inventory.input_devices,route.capture_id).available())return NativeAudioRouteStatus::MissingCapture;
 if(!fixed_native_audio_selection(inventory.output_devices,route.playback_id).available())return NativeAudioRouteStatus::MissingPlayback;
 return NativeAudioRouteStatus::Ready;
}
inline const wchar_t* native_audio_route_feedback(NativeAudioRouteStatus status){
 switch(status){
 case NativeAudioRouteStatus::Ready:return L"Rota de audio disponivel; audibilidade ainda exige teste com headset.";
 case NativeAudioRouteStatus::InvalidSelection:return L"Selecao de audio invalida. Pare o motor e selecione novamente.";
 case NativeAudioRouteStatus::InventoryFailed:return L"Falha ao consultar audio. Nenhuma chamada foi iniciada/atendida.";
 case NativeAudioRouteStatus::MissingCapture:return L"Microfone selecionado indisponivel. Reconecte ou pare o motor para trocar.";
 case NativeAudioRouteStatus::MissingPlayback:return L"Saida selecionada indisponivel. Reconecte ou pare o motor para trocar.";
 }
 return L"Audio indisponivel.";
}
}
