#include "../src/diagnostic_export.h"
#include <string>
int main(){
 axv::CallReadiness state{true,true,true,true,true,false,true};
 const auto text=axv::format_diagnostic_export(state);
 if(text.find(L"0.83.0")==text.npos || text.find(L"Botao Chamar: disponivel na interface")==text.npos ||
    text.find(L"RTP/SRTP nao verificado")==text.npos || text.find(L"CHECKLIST PENDENTE")==text.npos)return 1;
 if(text.find(L"ACEITE DE SOFTWARE:") == text.npos ||
    text.find(L"HOMOLOGACAO EXTERNA:") == text.npos ||
    text.find(L"PRODUCAO: BLOQUEADA") == text.npos) return 15;
 if(text.find(L"\r\n")==text.npos || text.find(L'\0')!=text.npos)return 2;
 for(std::size_t i=0;i<text.size();++i){
  if(text[i]==L'\n' && (!i || text[i-1]!=L'\r'))return 3;
  if(text[i]==L'\r' && (i+1==text.size() || text[i+1]!=L'\n'))return 4;
 }
 const auto empty=axv::format_diagnostic_export({});
 if(empty.find(L"Botao Chamar: indisponivel na interface")==empty.npos || empty.find(L"sem evidencia real de registro")==empty.npos)return 5;
 for(const auto* value:{L"sip:",L"sips:",L"@",L"password=",L"endpoint_id",L"127.0.0.1"})if(text.find(value)!=text.npos)return 6;
 // Exhaust the 7 readiness booleans and 4 security flags: 2,048
 // deterministic diagnostic combinations. An available dial button or
 // selected TLS policy must never imply confirmed media or SIP registration.
 for(unsigned readiness=0;readiness<128;++readiness) {
  const axv::CallReadiness state2{
    bool(readiness&1u),bool(readiness&2u),bool(readiness&4u),
    bool(readiness&8u),bool(readiness&16u),bool(readiness&32u),
    bool(readiness&64u)
  };
  const bool expectedGates=(readiness&63u)==63u;
  for(unsigned policy=0;policy<16;++policy) {
   const axv::NativeSecurityStatus security{
     bool(policy&1u),bool(policy&2u),bool(policy&4u),bool(policy&8u)
   };
   const auto report=axv::format_diagnostic_export(state2,security);
   if(report.empty() || report.find(L'\0')!=report.npos)return 7;
   const bool gates=report.find(L"GATES LOCAIS COMPLETOS")!=report.npos;
   if(gates!=expectedGates)return 8;
   const bool registration=report.find(L"Registro SIP atual: verificado")!=report.npos;
   if(registration != (security.policy_selected&&security.running_process&&security.verified_registration))return 9;
   if(report.find(L"homologacao real ainda necessaria")==report.npos &&
      report.find(L"homologacao nao concluida")==report.npos)return 10;
   if(report.find(L"Midia: este candidato nao oferece SRTP")==report.npos)return 11;
   for(const auto* forbidden:{L"secret",L"password=",L"passwd=",L"auth_pass",
     L"sip:",L"sips:",L"@",L"endpoint_id",L"127.0.0.1",L"TLS ativo",L"SRTP verificado"}) {
      if(report.find(forbidden)!=report.npos)return 12;
   }
   for(std::size_t i=0;i<report.size();++i) {
    if(report[i]==L'\n'&&(!i||report[i-1]!=L'\r'))return 13;
    if(report[i]==L'\r'&&(i+1==report.size()||report[i+1]!=L'\n'))return 14;
   }
  }
 }
 return 0;
}
