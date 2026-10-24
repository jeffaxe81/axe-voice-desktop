#pragma once
#include "sip_engine.h"
namespace axv {
enum class SecurityLevel { Plaintext, SignalingTls };
inline SecurityLevel account_security_level(const SipAccount& account) {
    return account.use_tls ? SecurityLevel::SignalingTls : SecurityLevel::Plaintext;
}
// TLS protects SIP signaling only. Media requires an independently verified SRTP policy.
inline bool is_media_encryption_confirmed(const SipAccount&) { return false; }
}
