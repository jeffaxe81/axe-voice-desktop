#pragma once
#include "sip_account.h"
#include <string>

// No passwords, secrets, transport probes or network requests.
// These results are about syntax only; they do not prove a SIP registration.
namespace axv {
enum class AccountFeedbackKind { Incomplete, Invalid, Valid };
struct AccountFormFeedback {
    AccountFeedbackKind kind;
    AccountError error;
    unsigned short port;
    const wchar_t* message;
    bool valid() const { return kind == AccountFeedbackKind::Valid; }
};
inline AccountFormFeedback evaluate_account_form(const std::wstring& server,
                                                 const std::wstring& username,
                                                 const std::wstring& port_text) {
    if (server.empty()) {
        return {AccountFeedbackKind::Incomplete, AccountError::MissingServer, 0,
                L"Informe o servidor SIP (validacao local, sem registro)."};
    }
    if (username.empty()) {
        return {AccountFeedbackKind::Incomplete, AccountError::MissingUser, 0,
                L"Informe o usuario SIP (validacao local, sem registro)."};
    }
    if (port_text.empty()) {
        return {AccountFeedbackKind::Incomplete, AccountError::InvalidPort, 0,
                L"Informe a porta SIP (1 a 65535)."};
    }
    unsigned long parsed_port = 0;
    for (const wchar_t digit : port_text) {
        if (digit < L'0' || digit > L'9') {
            return {AccountFeedbackKind::Invalid, AccountError::InvalidPort, 0,
                    L"Porta SIP invalida: use apenas numeros."};
        }
        parsed_port = parsed_port * 10 + static_cast<unsigned long>(digit - L'0');
        if (parsed_port > 65535) {
            return {AccountFeedbackKind::Invalid, AccountError::InvalidPort, 0,
                    L"Porta SIP fora do intervalo 1 a 65535."};
        }
    }
    if (parsed_port == 0) {
        return {AccountFeedbackKind::Invalid, AccountError::InvalidPort, 0,
                L"Porta SIP invalida: use 1 a 65535."};
    }
    const SipAccount candidate{server, username, static_cast<unsigned short>(parsed_port), false};
    const AccountError error = validate_account(candidate);
    if (error == AccountError::InvalidServer) {
        return {AccountFeedbackKind::Invalid, error, 0,
                L"Servidor SIP invalido: use nome DNS ou IP, sem espacos."};
    }
    if (error == AccountError::InvalidUser) {
        return {AccountFeedbackKind::Invalid, error, 0,
                L"Usuario SIP invalido: remova espacos e caracteres especiais."};
    }
    if (error != AccountError::None) {
        return {AccountFeedbackKind::Invalid, error, 0,
                L"Conta SIP invalida; revise servidor, usuario e porta."};
    }
    return {AccountFeedbackKind::Valid, AccountError::None,
            static_cast<unsigned short>(parsed_port),
            L"Dados validos localmente. Registro SIP nao realizado."};
}
} // namespace axv
