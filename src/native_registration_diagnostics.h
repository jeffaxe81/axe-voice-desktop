#pragma once
// A pointer-free, credential-free diagnostic projection of the native
// registration *control plane*. This is NOT SIP registration proof.
// Never map ConfirmedByControlPlane to SipEvidence::VerifiedNativeApi or
// enable the call buttons. Run on ONE consumer thread only.
#include "baresip_native_registration_host.h"
#include <cstdint>

namespace axv {

enum class NativeRegistrationDiagnostic {
    Unavailable,
    CoreStarting,
    AccountPrepared,
    RegistrationSubmitted,
    CallbackConfirmedUnverified,
    Cancelled,
    DeadlineExceeded,
    Faulted,
    Revoked,
    StaleGeneration
};

struct NativeRegistrationDiagnosticView {
    NativeRegistrationDiagnostic state = NativeRegistrationDiagnostic::Unavailable;
    std::uint64_t generation = 0;
    std::uint64_t owner_ticks = 0;
    // Deliberately NO UA address, SIP URI, password, call ID, or API proof.
    bool allows_calls = false;
    bool verified_sip_registration = false;
};

class NativeRegistrationDiagnosticProjection {
public:
    NativeRegistrationDiagnosticView observe(
        const NativeRegistrationHostSnapshot& source,
        std::uint64_t requested_generation) {
        // Never let old/cancelled generations become usable again.
        if (!source.running || source.reactor_fault != LibreReactorFault::None ||
            !source.command_generation || !requested_generation) {
            if (source.command_generation)
                rejected_generation_ = source.command_generation;
            return make(source,
                source.reactor_fault != LibreReactorFault::None
                    ? NativeRegistrationDiagnostic::Faulted
                    : NativeRegistrationDiagnostic::Unavailable);
        }
        if (source.command_generation != requested_generation ||
            (last_generation_ && source.command_generation < last_generation_))
            return make(source, NativeRegistrationDiagnostic::StaleGeneration);

        if (source.command_generation == rejected_generation_)
            return make(source, NativeRegistrationDiagnostic::Revoked);

        if (last_generation_ != source.command_generation)
            last_generation_ = source.command_generation;

        if (!source.account_provisioned)
            return make(source, NativeRegistrationDiagnostic::CoreStarting);

        switch (source.last_dispatch) {
        case RegistrationDispatchKind::Idle:
        case RegistrationDispatchKind::Pending:
            return make(source, NativeRegistrationDiagnostic::AccountPrepared);
        case RegistrationDispatchKind::Submitted:
            return make(source, NativeRegistrationDiagnostic::RegistrationSubmitted);
        case RegistrationDispatchKind::Confirmed:
            // Even with callback+ua_isregistered, the UI does not have an
            // independently verified adapter or authenticated provenance.
            return make(source,
                NativeRegistrationDiagnostic::CallbackConfirmedUnverified);
        case RegistrationDispatchKind::Cancelled:
            return revoke(source, NativeRegistrationDiagnostic::Cancelled);
        case RegistrationDispatchKind::DeadlineExpired:
            return revoke(source, NativeRegistrationDiagnostic::DeadlineExceeded);
        case RegistrationDispatchKind::Failed:
        case RegistrationDispatchKind::RegistrarRejected:
        case RegistrationDispatchKind::OverflowStopped:
        case RegistrationDispatchKind::WrongOwnerThread:
        case RegistrationDispatchKind::SupersededDuringDispatch:
            return revoke(source, NativeRegistrationDiagnostic::Faulted);
        case RegistrationDispatchKind::StaleGeneration:
            return revoke(source, NativeRegistrationDiagnostic::StaleGeneration);
        }
        return revoke(source, NativeRegistrationDiagnostic::Faulted);
    }

    void reset() {
        last_generation_ = 0;
        rejected_generation_ = 0;
    }

private:
    NativeRegistrationDiagnosticView make(
        const NativeRegistrationHostSnapshot& source,
        NativeRegistrationDiagnostic kind) const {
        NativeRegistrationDiagnosticView out;
        out.state = kind;
        out.generation = source.command_generation;
        out.owner_ticks = source.ticks;
        // Safety invariants: diagnostics cannot authorize SIP call controls.
        out.allows_calls = false;
        out.verified_sip_registration = false;
        return out;
    }

    NativeRegistrationDiagnosticView revoke(
        const NativeRegistrationHostSnapshot& source,
        NativeRegistrationDiagnostic kind) {
        rejected_generation_ = source.command_generation;
        return make(source, kind);
    }

    std::uint64_t last_generation_ = 0;
    std::uint64_t rejected_generation_ = 0;
};

inline const wchar_t* native_registration_diagnostic_label(
    NativeRegistrationDiagnostic state) {
    switch (state) {
    case NativeRegistrationDiagnostic::Unavailable: return L"Motor SIP inativo";
    case NativeRegistrationDiagnostic::CoreStarting: return L"Preparando conta SIP";
    case NativeRegistrationDiagnostic::AccountPrepared: return L"Conta preparada (nao registrada)";
    case NativeRegistrationDiagnostic::RegistrationSubmitted: return L"Registro solicitado (aguardando)";
    case NativeRegistrationDiagnostic::CallbackConfirmedUnverified:
        return L"Callback de registro observado (homologacao pendente)";
    case NativeRegistrationDiagnostic::Cancelled: return L"Registro cancelado";
    case NativeRegistrationDiagnostic::DeadlineExceeded: return L"Prazo de registro excedido";
    case NativeRegistrationDiagnostic::Faulted: return L"Falha de controle SIP";
    case NativeRegistrationDiagnostic::Revoked: return L"Sessao nativa revogada";
    case NativeRegistrationDiagnostic::StaleGeneration: return L"Geracao antiga rejeitada";
    }
    return L"Estado SIP indisponivel";
}

} // namespace axv
