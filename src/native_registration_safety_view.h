#pragma once
// Native control-plane diagnostic bridge; NEVER an auth or call readiness grant.
// SDK types and live UA pointers never appear in the output.
#include "native_registration_diagnostics.h"

namespace axv {
struct NativeRegistrationSafetyView {
    NativeRegistrationDiagnosticView control_plane{};
    bool permits_gui_registration_status = false;
    bool permits_gui_calls = false;
    bool permits_media = false;
    const wchar_t* status = L"Sem produtor nativo conectado";
};

class NativeRegistrationSafetyProjection {
public:
    NativeRegistrationSafetyView observe(
        const NativeRegistrationHostSnapshot& snapshot,
        std::uint64_t expected_generation) {
        auto projected = projection_.observe(snapshot, expected_generation);
        NativeRegistrationSafetyView view;
        view.control_plane = projected;
        view.status = native_registration_diagnostic_label(projected.state);
        // A native callback observed in this experimental library does not
        // establish certificate policy, authenticated registrar, or RTP/SRTP.
        view.permits_gui_registration_status = false;
        view.permits_gui_calls = false;
        view.permits_media = false;
        return view;
    }

    void reset() { projection_.reset(); }
private:
    NativeRegistrationDiagnosticProjection projection_;
};
} // namespace axv
