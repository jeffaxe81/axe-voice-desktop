#pragma once
#include "native_destination_result.h"
#include "sip_runtime_observation.h"
namespace axv {
// Owned under the engine mutex. A refusal belongs to the displayed call/session,
// and must not obscure the result of a later admitted command.
class NativeDestinationFeedback {
public:
    void record(NativeDestinationError error,const SipRuntimeObservation& view) {
        error_=error;session_=view.session;call_=view.call_id;
    }
    void synchronize(const SipRuntimeObservation& view) {
        if(view.session!=session_ || view.call_id!=call_)clear();
    }
    void command_accepted(){clear();}
    void clear(){error_=NativeDestinationError::None;}
    NativeDestinationError error()const{return error_;}
private:
    NativeDestinationError error_=NativeDestinationError::None;
    std::uint64_t session_=0,call_=0;
};
}
