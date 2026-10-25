> **Registro documental — não é uma implementação na main.** Esta nota descreve a branch Baresip experimental de desenvolvimento; os arquivos-fonte da v0.130 não foram mesclados na base principal. Nenhum build, teste ou homologação foi executado nesta rodada.

# AXE Voice LAB — v0.126–v0.130: barreiras de evidência de REGISTER

Data da rodada: 24/10/2026. **Não foram realizados testes, compilação, execução de SDK ou PABX.** Esta evolução permanece no módulo de registro experimental, desativado por padrão no produto Windows.

## Cinco mudanças de implementação

| Versão | Implementação | Restrição |
| --- | --- | --- |
| 0.126 | O adaptador nativo `BaresipSdkEventAdapter` exige evento `BEVENT_REGISTERING` do UA selecionado antes de admitir `BEVENT_REGISTER_OK`. Callback OK prematuro, duplicado ou de outra fase revoga a época. | Esta política deliberadamente restritiva pode rejeitar fluxos automáticos legítimos de nova inscrição do Baresip. Uma nova época explícita é exigida. |
| 0.127 | `BEVENT_REGISTER_FAIL` torna-se sinal terminal e persistente **na mesma época**, sem ser apagado por um novo `BEVENT_REGISTERING`. `UNREGISTERING` encerra a época, e callbacks posteriores não reabrem registro. | O cofre local e a ausência de evento de falha jamais certificam autenticação. |
| 0.128 | `BaresipSdkRegistrationGateway::submit` detecta `REGISTER_FAIL` disparado sincronamente pelo SDK dentro de `ua_register()`, mesmo quando a chamada C retorna zero, antes de admitir o registro ou armar timeout. | O resultado registra rejeição do registrador, não estabelece operação SIP. |
| 0.129 | `BaresipRegistrationCommandDispatcher` conserva a classificação `RegistrarRejected` tanto para erro síncrono quanto para rejeição percebida enquanto arma o deadline; encerra gateway, revoga fila e impede reaproveitamento da geração. | O controlador mantém seus estados de falha/retentativa conservadores. |
| 0.130 | Depois de `REGISTER_OK` confirmado, o gateway verifica periodicamente se a prova nativa ainda existe. Ao desaparecer sem uma notificação segura, revoga a sessão e informa `SessionRevoked`, evitando conservar confirmação antiga. Versões da GUI/CMake e README atualizadas para 0.130. | A confirmação local é apenas uma evidência experimental; não autoriza chamada, TLS ou SRTP. |

## Segurança e homologação

A validação estrutural da diff e a existência de código **não equivalem** a compilação, teste, registro SIP autenticado ou aceite. O `AXEVoice.exe` continua com motor indisponível (`UnavailableSipEngine`) e `SipEvidence::None`; o alvo Baresip, a conexão WinCred e a biblioteca nativa permanecem **OFF por padrão**.

Próximos pré-requisitos de operação: revisar SDK/ABI/código-fonte e memória de segredos, compilar no MSVC, homologar gerenciamento de eventos do Baresip (incluindo renovações de REGISTER), verificar TLS e certificados, SIP REGISTER 401/403/200 em PABX real, SRTP/codec, chamada recebida e originada e áudio bidirecional nos equipamentos Windows. Não mesclar executáveis experimentais à `main` sem critérios de aceite.

Esta rodada segue a instrução **sem executar testes**. A documentação na `main` pode registrar o estado, mas não deve afirmar que foi liberado um motor SIP funcional.
