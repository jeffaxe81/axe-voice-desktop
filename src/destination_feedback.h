#pragma once
#include "sip_address.h"
#include <string>

namespace axv {
// Presentation-only validation. A valid destination is NOT evidence of a registered
// account, an authenticated engine, or an established media path.
enum class DestinationFeedbackKind { Empty, Invalid, Valid };
struct DestinationFeedback {
    DestinationFeedbackKind kind;
    AddressError error;
    const wchar_t* message;
    bool valid() const { return kind == DestinationFeedbackKind::Valid; }
};
inline DestinationFeedback evaluate_destination(const std::wstring& input) {
    const AddressResult result = validate_address(input);
    switch (result.error) {
    case AddressError::None:
        return {DestinationFeedbackKind::Valid, AddressError::None,
            input.find(L'@') == std::wstring::npos
                ? L"Ramal valido. Chamada depende de registro SIP real."
                : L"Endereco SIP valido. Chamada depende de registro SIP real."};
    case AddressError::Empty:
        return {DestinationFeedbackKind::Empty, result.error,
                L"Digite um ramal ou endereco SIP."};
    case AddressError::Whitespace:
        return {DestinationFeedbackKind::Invalid, result.error,
                L"Remova espacos e caracteres de controle do destino."};
    case AddressError::InvalidScheme:
        return {DestinationFeedbackKind::Invalid, result.error,
                L"Use ramal ou URI sip:/sips: (nao HTTP/HTTPS)."};
    case AddressError::InvalidHost:
        return {DestinationFeedbackKind::Invalid, result.error,
                L"Verifique o dominio e a posicao do @."};
    case AddressError::InvalidPort:
        return {DestinationFeedbackKind::Invalid, result.error,
                L"Porta SIP invalida (1 a 65535)."};
    case AddressError::InvalidUser:
        return {DestinationFeedbackKind::Invalid, result.error,
                L"Ramal ou usuario SIP invalido."};
    }
    return {DestinationFeedbackKind::Invalid, result.error,
            L"Destino SIP invalido."};
}
} // namespace axv
