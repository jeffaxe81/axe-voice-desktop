# AXE Voice v0.64 — primeira consolidação da infraestrutura SIP nativa

Base da interface: v0.63 (PR #78). Origem das rotinas experimentais: ramo `feature/libre-registration-owner-integration` (PR #70).

## Objetivo desta entrega

Consolidar componentes nativos já exercitados isoladamente, sem fazer o aplicativo assumir que uma chamada SIP está operacional.

- `BaresipNativeLibreReactor`: proprietário exclusivo de `libre_init`, `re_main`, `mqueue`, timer nativo e `libre_close` em uma única thread; parada via sinal cooperativo e confirmação por `join`.
- `RegistrationCommandQueue`: fila pequena de comandos sem ponteiros, sem credenciais ou URI SIP; token de geração e revisão invalidam comandos após cancelamento.
- `RegistrationDeadline`: watchdog monotônico para impedir que um pedido de registro permaneça indefinidamente aguardando resposta.
- `BaresipNativeThreadGuard`: verificação explícita da thread proprietária das operações nativas.
- Quatro novos CTests de regressão: `libre_owner_reactor` (SDK livre simulado, thread real), `baresip_registration_command_queue`, `baresip_registration_watchdog` e `baresip_native_thread_guard`.

Os arquivos estão na mesma árvore de desenvolvimento Windows/frontend, mas **não são vinculados ao executável `AXE-Voice.exe`**, nem ligam o SDK real ou emitem `VerifiedNativeApi`. A linha de frontend mantém os gates de SIP, de campainha, de chamada e de mídia fechados. Este incremento não cria conta UA nem configura senha SIP.

## Próxima integração técnica

Antes de conectar o host nativo à UI, reconciliar de modo auditável as diferenças de `baresip_sdk_event_adapter.h` e `baresip_callback_lifecycle.h` entre as duas linhas. Não substituir a versão mais recente do frontend por uma implementação antiga do ramo SIP: seriam perdidas proteções contra eventos incorretos, callbacks atrasados e reutilização de identidades de chamada. Após essa reconciliação, ligar um `BaresipNativeRegistrationHost` à interface apenas com registro real comprovado, testes de integração e SDK pinado por hash.

## Homologação ainda obrigatória

CI MSVC Release e CTest; SDK Baresip/libre real auditado; registro e duas chamadas bidirecionais em PABX; TLS/SRTP; áudio USB/Bluetooth; assinatura da distribuição. Resultados de testes com `tests/fakes/re.h` **não** comprovam SIP/RTP em produção.
