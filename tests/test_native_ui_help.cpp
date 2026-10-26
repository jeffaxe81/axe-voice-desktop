#define AXV_ENABLE_NATIVE_SIP
#include "../src/ui_help.h"
#include <string_view>
int main(){
 const std::wstring_view help=axv::frontend_help_text();
 return help.find(L"REGISTRAR")!=help.npos && help.find(L"CA PEM")!=help.npos &&
        help.find(L"padrao de comunicacao")!=help.npos &&
        help.find(L"primeira observacao de chamada estabelecida")!=help.npos &&
        help.find(L"Inclui espera; nao mede qualidade nem audio")!=help.npos &&
        help.find(L"permanecem bloqueados")==help.npos ? 0:1;
}
