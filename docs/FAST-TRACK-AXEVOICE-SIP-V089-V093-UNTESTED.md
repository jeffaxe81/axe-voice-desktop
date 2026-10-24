# AXE Voice — ciclo SIP de desenvolvimento v0.89–v0.93

Data: 24/10/2026. **Cinco mudanças de código publicadas sem compilar ou executar testes nesta rodada** (orientação anterior). O CI existente pode rodar automaticamente por causa dos commits; não supor resultados.

## Entregas

| Versão | Entrega | Implementação e limite |
|---|---|---|
| 0.89 | Verificação de cofre Windows | `src/sip_vault_status_win32.h` monta um identificador de credencial genérica versionado para a combinação de usuário e domínio DNS, consulta a presença com `CredReadW`, limpa e libera os bytes sem copiá-los nem mostrá-los. Detectar presença **não confirma senha correta ou autenticação**. |
| 0.90 | Política de novas tentativas | `src/native_registration_retry_budget.h` implementa limite de cinco falhas de submissão local e intervalo progressivo de 1, 2, 4, 8 e 16 segundos baseado em relógio monotônico. Não dispara tentativas automáticas. |
| 0.91 | Controlador experimental | `src/experimental_native_registration_controller.h/.cpp` liga **start, submit, cancel e stop** aos métodos já existentes de `BaresipNativeRegistrationHost`, trava geração, recusa submissão simultânea, reconhece falha de submissão e exige nova sessão após cancelamento. Só está no target nativo de laboratório. |
| 0.92 | Leitura conservadora do estado | `NativeControlView` expõe presença de conta, geração e última decisão do controlador sem ponteiros ou credenciais; `sip_authenticated=false` e `media_verified=false` em todos os snapshots. Mesmo `Confirmed` significa apenas callback no plano de controle. |
| 0.93 | Fluxo de cofre no frontend | Menu MAIS e F4 mostram existência da credencial e, **por ação explícita**, seu identificador contendo usuário/domínio para configuração manual; F5 abre o Gerenciador de Credenciais do Windows. Sem exibir senha, consumir segredo no SDK ou registrar chamadas. |

**Também realizado:** CMake 0.93.0; a biblioteca opcional `axv_native_registration_lab` agora inclui o controlador e o executável auxiliar de *link probe* referencia seus símbolos (somente quando explicitamente habilitado). `AXV_BUILD_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME` continua OFF por padrão. O executável `AXEVoice.exe` não vincula o SDK nativo.

## Pontos de atenção

- O cofre precisa de uma credencial genérica cadastrada pelo próprio usuário do Windows. O identificador é exibido apenas sob comando F4 e contém o usuário e o domínio da conta; isso é informação de identificação local, mas não inclui a senha. Evite compartilhar capturas dessa janela.
- As falhas contabilizadas no orçamento atual são **rejeições locais de submissão**. Falhas reais do registrador SIP e autenticação ainda não têm integração completa ao controlador nem retentativa automática. Não confundir orçamento com política final de reconexão.
- O controlador de laboratório exige SDK/libre auditados, perfil isolado, opt-in e lifecycle privado. **Ele não recebe nem aplica credenciais do cofre ao Baresip.**
- Para uma chamada real faltam o provedor de autenticação SDK, negociação TLS/SRTP, ponte de eventos verificados à GUI e validação de áudio/dispositivos.
- Pendências de homologação: compilação MSVC Windows, teste de botão F4/F5, segredo genérico válido/inválido, proprietários de thread, race de cancelamento, timeout, reconexão, PABX real, chamada de entrada/saída, RTP bidirecional PC/headset e segurança. Nenhuma prova foi marcada como executada.
- A `main` não é alterada nesta entrega. Não remover o bloqueio de produção nem declarar SIP operacional.

Documentação técnica complementa `docs/FAST-TRACK-AXEVOICE-SIP-V084-V088-UNTESTED.md`.
