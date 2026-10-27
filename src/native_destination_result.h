#pragma once
#include <string>
namespace axv {
enum class NativeDestinationError { None, InvalidAddress, TransportMismatch, UnsupportedCharacters, TooLong, InvalidAccount };
struct NativeDestinationResult {
    NativeDestinationError error=NativeDestinationError::InvalidAddress;
    std::string uri;
    explicit operator bool() const { return error==NativeDestinationError::None && !uri.empty(); }
};
// Static error-only feedback. Never echo destination, account or certificate data.
inline const wchar_t* native_destination_error_message(NativeDestinationError error) {
    switch(error) {
    case NativeDestinationError::None:return L"";
    case NativeDestinationError::InvalidAddress:return L"Destino recusado: revise o ramal, usuario, dominio e porta SIP.";
    case NativeDestinationError::TransportMismatch:return L"Destino recusado: SIP/SIPS deve combinar com o transporte da conta. Use um ramal sem esquema ou confira TLS.";
    case NativeDestinationError::UnsupportedCharacters:return L"Destino recusado: use caracteres ASCII suportados, sem escapes percentuais ou parametros extras.";
    case NativeDestinationError::TooLong:return L"Destino recusado: a URI final deve ter no maximo512 bytes, incluindo a rota da conta.";
    case NativeDestinationError::InvalidAccount:return L"Destino recusado: revise a conta configurada antes de discar um ramal.";
    }
    return L"Destino recusado: revise os dados antes de tentar novamente.";
}
}
