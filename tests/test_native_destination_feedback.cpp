#include "../src/native_destination_feedback.h"
#include <cstdlib>
#define CHECK(x) do { if(!(x))std::abort(); } while(0)
int main(){
 using namespace axv;
 NativeDestinationFeedback feedback;
 SipRuntimeObservation first{RuntimeProcess::Running,SipStatus::InCall,SipEvidence::VerifiedNativeApi,7,11,11};
 feedback.record(NativeDestinationError::TransportMismatch,first);
 feedback.synchronize(first);
 CHECK(feedback.error()==NativeDestinationError::TransportMismatch);
 // A later admitted action must allow its mute/hold/DTMF result to be shown.
 feedback.command_accepted();
 CHECK(feedback.error()==NativeDestinationError::None);
 for(auto next:{SipRuntimeObservation{RuntimeProcess::Running,SipStatus::Ringing,SipEvidence::VerifiedNativeApi,7,12,12},
                SipRuntimeObservation{RuntimeProcess::Running,SipStatus::InCall,SipEvidence::VerifiedNativeApi,8,11,11},
                SipRuntimeObservation{RuntimeProcess::Running,SipStatus::Registered,SipEvidence::VerifiedNativeApi,7,0,0}}){
  feedback.record(NativeDestinationError::UnsupportedCharacters,first);
  feedback.synchronize(next);
  CHECK(feedback.error()==NativeDestinationError::None);
 }
 feedback.record(NativeDestinationError::TooLong,first);
 feedback.record(NativeDestinationError::None,first);
 CHECK(feedback.error()==NativeDestinationError::None);
 feedback.record(NativeDestinationError::InvalidAddress,first);
 feedback.clear();
 CHECK(feedback.error()==NativeDestinationError::None);
}
