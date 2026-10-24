#include "../src/baresip_sdk_abi_contract.h"
#include <string_view>
int main() {
    // This is a FAKE header compile test, not evidence that a real SDK links.
    static_assert(std::string_view(BARESIP_VERSION)=="4.12.0");
    static_assert(sizeof(void*)==sizeof(struct ua*));
    return 0;
}
