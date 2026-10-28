// Pure Windows CRT argument quoting regression; no Windows runtime required.
#include "../src/windows_argument_quote.h"
#include <iostream>
#include <string>
#include <string_view>

int main() {
    struct Case { std::wstring_view input; std::wstring_view expected; };
    const Case cases[] = {
        {L"", L"\"\""},
        {L"plain", L"\"plain\""},
        {L"Voice profiles", L"\"Voice profiles\""},
        {L"Voz \u03a9 \u65e5\u672c\u8a9e", L"\"Voz \u03a9 \u65e5\u672c\u8a9e\""},
        {LR"(C:\)", LR"("C:\\")"},
        {LR"(C:\Voice profiles\)", LR"("C:\Voice profiles\\")"},
        {LR"(C:\Voice\\)", LR"("C:\Voice\\\\")"},
        {LR"(a"b)", LR"("a\"b")"},
        {LR"(a\"b)", LR"("a\\\"b")"},
        {LR"(a\\"b)", LR"("a\\\\\"b")"},
        {LR"(C:\Voice & echo harmless)", LR"("C:\Voice & echo harmless")"},
        {LR"(C:\a\\b)", LR"("C:\a\\b")"},
    };
    for (const auto& test : cases) {
        const auto actual = axv::quote_windows_argument(test.input);
        if (actual != test.expected) {
            std::wcerr << L"quote_windows_argument mismatch for [" << test.input
                       << L"]\nexpected [" << test.expected << L"]\nactual ["
                       << actual << L"]\n";
            return 1;
        }
    }
    return 0;
}
