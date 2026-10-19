#pragma once
#include "sip_engine.h"
namespace axv {
inline const wchar_t* sip_status_label(SipStatus s) {
    switch(s) {
    case SipStatus::Unavailable: return L"Motor indisponível";
    case SipStatus::Disconnected: return L"Desconectado";
    case SipStatus::Registering: return L"Registrando";
    case SipStatus::Registered: return L"Registrado";
    case SipStatus::Calling: return L"Chamando";
    case SipStatus::InCall: return L"Em chamada";
    case SipStatus::Failed: return L"Falha SIP";
    }
    return L"Estado desconhecido";
}
}
