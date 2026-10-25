#include <windows.h>
#include <cstddef>
#include <cstring>
int main() {
    char payload[sizeof("/quit\n")] = {};
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (!input || input == INVALID_HANDLE_VALUE) return 3;
    for (size_t pos = 0; pos < sizeof("/quit\n") - 1;) {
        DWORD bytes = 0;
        if (!ReadFile(input, payload + pos,
            static_cast<DWORD>(sizeof("/quit\n") - 1 - pos), &bytes, nullptr) || !bytes) return 4;
        pos += bytes;
    }
    return std::memcmp(payload, "/quit\n", sizeof("/quit\n") - 1) ? 5 : 0;
}
