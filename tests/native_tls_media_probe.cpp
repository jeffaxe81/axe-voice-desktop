#include "../src/baresip_sdk_headers.h"
#include <openssl/ssl.h>
#include <cstring>
extern "C" {
extern const struct mod_export exports_g711;
extern const struct mod_export exports_wasapi;
}
// Static references require the actual compiled module objects to link.
static const struct mod_export* volatile codec = &exports_g711;
static const struct mod_export* volatile devices = &exports_wasapi;
int main() {
    if (!codec || !devices || std::strcmp(codec->name,"g711") ||
        std::strcmp(devices->name,"wasapi")) return 1;
    // Check the provider context locally. No socket, handshake or device open.
    SSL_CTX* context = SSL_CTX_new(TLS_client_method());
    if (!context) return 2;
    const int configured = SSL_CTX_set_min_proto_version(context,TLS1_2_VERSION);
    SSL_CTX_set_verify(context,SSL_VERIFY_PEER,nullptr);
    const bool verified = configured == 1 &&
        SSL_CTX_get_min_proto_version(context) == TLS1_2_VERSION &&
        (SSL_CTX_get_verify_mode(context) & SSL_VERIFY_PEER) != 0;
    SSL_CTX_free(context);
    return verified ? 0 : 3;
}
