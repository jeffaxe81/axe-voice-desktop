#pragma once
#include "call_readiness.h"
#include <string>

namespace axv {
// The input has only boolean states; raw SIP/account/audio values cannot be
// passed into this formatter or included in the clipboard payload.
inline std::wstring format_diagnostic_export(const CallReadiness& readiness) {
    std::wstring report =
        L"AXE Voice Desktop v0.40 - diagnostico local (laboratorio)\n"
        L"Somente estados de prontidao; sem campos identificadores.\n"
        L"Nao contem servidor, usuario, senha, destino ou IDs de headset.\n\n";
    report += format_call_readiness(readiness);
    std::wstring out;
    out.reserve(report.size() + 32);
    for (std::size_t i = 0; i < report.size(); ++i) {
        if (report[i] == L'\n' && (i == 0 || report[i-1] != L'\r'))
            out += L'\r';
        out += report[i];
    }
    return out;
}
} // namespace axv
