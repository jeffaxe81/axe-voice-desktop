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
    auto plan = axv::plan_dial_edit(L"1234", 2, 2, L'5');
    if (!plan.accepted || plan.start != 2 || plan.end != 2 || plan.replacement != L"5") return 9;
    plan = axv::plan_dial_edit(L"12534", 1, 4, L'\b');
    if (!plan.accepted || plan.start != 1 || plan.end != 4 || !plan.replacement.empty()) return 10;
    plan = axv::plan_dial_edit(L"1001", 0, 0, L'\b');
    if (plan.accepted) return 11;
    plan = axv::plan_dial_edit(L"1234", 0, 0, L'\0');
    if (!plan.accepted || plan.start != 0 || plan.end != 4 || !plan.replacement.empty()) return 12;
    plan = axv::plan_dial_edit(L"123", 1, 2, L'4', 3);
    if (!plan.accepted || plan.start != 1 || plan.end != 2) return 13;
    plan = axv::plan_dial_edit(L"123", 3, 3, L'4', 3);
    if (plan.accepted) return 14;
    plan = axv::plan_dial_edit(L"123", 1, 1, L'@');
    if (plan.accepted) return 15;
    plan = axv::plan_dial_edit(L"123", 50, 60, L'0');
    if (!plan.accepted || plan.start != 3 || plan.end != 3) return 16;
    return 0;

