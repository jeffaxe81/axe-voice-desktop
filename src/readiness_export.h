#pragma once
#include "call_readiness.h"
#include <string>
namespace axv {
// Reconciled from draft PR #21. Six Boolean gates only: no SIP identity,
// dialed number, credential, server, IP path or audio endpoint ID is read.
// This is an informational report, not authentication or a call permit.
inline std::wstring format_readiness_export(const CallReadiness& flags) {
    return L"AXE VOICE / RELATORIO DE SUPORTE (LAB)\n"
           L"Enderecos SIP e identificadores omitidos.\n\n" +
           format_call_readiness(flags) +
           L"\n\nEste documento NAO comprova registro SIP, RTP/SRTP "
           L"ou homologacao de producao.\n";
}
} // namespace axv
