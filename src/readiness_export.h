#pragma once
#include "call_readiness.h"
#include <string>
namespace axv {
// Six Boolean gates only: no SIP identity, dialed number, credential,
// server, network path or audio endpoint ID can enter this export.
inline std::wstring format_readiness_export(const CallReadiness& flags) {
    return L"AXE VOICE / RELATORIO DE SUPORTE (LAB)\n"
           L"Enderecos SIP e identificadores omitidos.\n\n" +
           format_call_readiness(flags) +
           L"\n\nEste documento NAO comprova registro SIP, RTP/SRTP "
           L"ou homologacao de producao.\n";
}
} // namespace axv
