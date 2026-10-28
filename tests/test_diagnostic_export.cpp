#include "../src/diagnostic_export.h"
#include <string>
int main(){
 axv::CallReadiness state{true,true,true,true,true,false,true};
 const auto text=axv::format_diagnostic_export(state);
 if(text.find(L"0.64.0")==text.npos || text.find(L"Botao Chamar: disponivel na interface")==text.npos ||
    text.find(L"RTP/SRTP nao verificado")==text.npos || text.find(L"CHECKLIST PENDENTE")==text.npos)return 1;
 if(text.find(L"\r\n")==text.npos || text.find(L'\0')!=text.npos)return 2;
 for(std::size_t i=0;i<text.size();++i){
  if(text[i]==L'\n' && (!i || text[i-1]!=L'\r'))return 3;
  if(text[i]==L'\r' && (i+1==text.size() || text[i+1]!=L'\n'))return 4;
 }
 const auto empty=axv::format_diagnostic_export({});
 if(empty.find(L"Botao Chamar: indisponivel na interface")==empty.npos || empty.find(L"sem evidencia real de registro")==empty.npos)return 5;
 for(const auto* value:{L"sip:",L"sips:",L"@",L"password=",L"endpoint_id",L"127.0.0.1"})if(text.find(value)!=text.npos)return 6;
 return 0;
}
