#pragma once
// In-memory, bounded diagnostic journal of VERIFIED SDK callbacks only.
// Never store SIP URIs, Caller-ID, credentials, audio or raw packet contents.
#include <array>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <vector>

namespace axv {
enum class MonitoredSipEvent {
    Registering, Registered, RegisterFailed, Unregistering,
    Incoming, Outgoing, Established, Closed, TransferFailed,
    AudioError, SessionRevoked
};
struct SipMonitorEntry {
    std::uint64_t sequence=0;
    std::time_t when=0;
    MonitoredSipEvent event=MonitoredSipEvent::Registering;
    std::uint64_t session=0;
    std::uint64_t call=0;
};
class SipCallMonitor {
public:
    static constexpr std::size_t capacity=128;
    void record(MonitoredSipEvent event,std::uint64_t session,std::uint64_t call=0) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(sequence_==UINT64_MAX)return; // Never reuse sequence IDs.
        entries_[(start_+count_)%capacity]={++sequence_,std::time(nullptr),event,session,call};
        if(count_<capacity)++count_;
        else start_=(start_+1)%capacity;
    }
    std::vector<SipMonitorEntry> snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<SipMonitorEntry> result;
        result.reserve(count_);
        for(std::size_t i=0;i<count_;++i)result.push_back(entries_[(start_+i)%capacity]);
        return result;
    }
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        start_=count_=0; // Keep sequence monotonic, even after clearing.
    }
private:
    mutable std::mutex mutex_;
    std::array<SipMonitorEntry,capacity> entries_{};
    std::size_t start_=0,count_=0;
    std::uint64_t sequence_=0;
};
inline const wchar_t* sip_monitor_event_name(MonitoredSipEvent event){
    switch(event){
    case MonitoredSipEvent::Registering:return L"Registro iniciado";
    case MonitoredSipEvent::Registered:return L"Registro confirmado";
    case MonitoredSipEvent::RegisterFailed:return L"Registro recusado";
    case MonitoredSipEvent::Unregistering:return L"Desregistro";
    case MonitoredSipEvent::Incoming:return L"CHAMADA RECEBIDA";
    case MonitoredSipEvent::Outgoing:return L"Chamada originada";
    case MonitoredSipEvent::Established:return L"Chamada estabelecida";
    case MonitoredSipEvent::Closed:return L"Chamada encerrada";
    case MonitoredSipEvent::TransferFailed:return L"Transferencia falhou";
    case MonitoredSipEvent::AudioError:return L"Falha de audio";
    case MonitoredSipEvent::SessionRevoked:return L"Sessao SIP revogada";
    }
    return L"Evento desconhecido";
}
}
