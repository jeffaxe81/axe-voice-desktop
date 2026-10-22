#include "../src/dialpad_model.h"
#include <string>
int main() {
    std::wstring number;
    for (wchar_t digit : axv::dialpad_keys()) {
        if (!axv::dial_append(number, digit)) return 1;
    }
    if (number != L"123456789*0#") return 2;
    axv::dial_backspace(number);
    if (number != L"123456789*0") return 3;
    if (axv::dial_append(number, L'\n') ||
        axv::dial_append(number, L';') ||
        axv::dial_append(number, L'@')) return 4;
    if (number != L"123456789*0") return 5;
    std::wstring small = L"123";
    if (axv::dial_append(small, L'4', 3)) return 6;
    if (small != L"123") return 7;
    small.clear();
    axv::dial_backspace(small);
    if (!small.empty()) return 8;
    return 0;
}
