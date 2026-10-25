> **Documento de acompanhamento:** este arquivo está na `main` apenas como registro da evolução da branch Baresip experimental. As fontes v0.125 descritas abaixo **não fazem parte do código executável desta `main`**; não foram testadas nem homologadas.

# AXE Voice LAB — v0.121–v0.125: proteção do ciclo de registro

Data da revisão: 24/10/2026. **Nenhum teste, compilação, execução Windows, SDK ou PABX nesta rodada.** São alterações de código revisadas apenas estruturalmente, não homologadas.

## Cinco entregas

| Versão | Mudança no código | Observação |
|---|---|---|
| 0.121 | Funções `registration_dispatch_terminal` e `registration_dispatch_failure` centralizam classificação das decisões do dispatcher nativo. | Cancelamento não é erro de autenticação. |
| 0.122 | `BaresipNativeRegistrationHost::snapshot()` expõe `accepting_requests` de modo seguro, sem senha, URI, ponteiro UA ou prova de SIP. O host preserva o primeiro resultado terminal da geração. | A flag representa admissão local de comandos, não registro no PABX. |
| 0.123 | `request_registration()` recusa uma geração sem admissão, com UA ausente ou reactor falho antes de enfileirar registro. | Exige recomeçar uma nova geração depois de falha terminal. |
| 0.124 | `cancel()` distingue cancelamento do usuário, geração já falhada ou encerrada e bloqueia cancelamento posterior de uma sessão terminal. | `CancellationRequested` indica apenas comando aceito para entrega, não limpeza concluída. |
| 0.125 | `NativeControlView` expõe `native_admission_open` e apresenta `register_request_pending` somente se host, geração, admissão e estado ainda forem válidos; `restart_required` passa a refletir também decisões terminais nativas ainda não consultadas. CMake/Win32 LAB v0.125 e README atualizado. | `sip_authenticated=false` e `media_verified=false`; nenhum comando de chamada liberado. |

## Pendente para operação
- Compilar no Windows 11 com Visual Studio/MSVC, SDK Baresip/libre auditados e pins SHA de binários, cabeçalhos e `src/account.c`; confirmar compatibilidade da API.
- Inspecionar encerramento da thread e limpeza do buffer da senha com instrumentação; reproduzir cenários concorrentes de cancelamento/timeout/registro rejeitado.
- Confirmar certificado TLS, autenticação real REGISTER 401/407/200, SRTP e chamadas bidirecionais, codec, headset/PC.
- Só após homologação e aceite explícito mudar o motor `UnavailableSipEngine` no produto, avaliar liberação de chamadas e promover à `main`.
- PRs antigas do laboratório empilhadas podem conter código já existente em desenvolvimento. Não fazer merge cego de PRs Draft apenas pela quantidade.

O código nativo Baresip permanece com opt-ins **OFF** por padrão, separado do `AXEVoice.exe`; esta documentação não representa execução de testes. O README na branch de desenvolvimento foi atualizado para deixar esses limites visíveis.
