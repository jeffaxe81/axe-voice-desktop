# Baresip integration gate

The desktop currently selects `UnavailableSipEngine` using `create_sip_engine()`.
This is deliberate: neither the Baresip SDK nor a native backend is currently linked.

## Requirements before enabling Baresip
1. Pin an audited Baresip/re release and record build instructions and licenses.
2. Implement `SipEngine` on a worker thread with safe event dispatch to the Win32 message loop.
3. Handle registration, network failures, incoming/outgoing calls, hangup and shutdown.
4. Securely handle credentials and avoid storing passwords in source code or logs.
5. Verify SIP over TLS and media SRTP independently.
6. Validate Windows audio devices and codec negotiation with test PBX.
7. Keep the fallback behavior until Windows CI and real calls pass.

There must be no successful-registration or connected-call claim when using the fallback.

## Preflight v0.17.0

O aplicativo audita localmente um conjunto fixo de nomes de DLL em modules/ ao lado de baresip.exe, e mostra códigos de saída de processo. Isso prepara o diagnóstico de empacotamento, mas **não** implementa o motor SIP, o carregamento real das DLLs nem canal de controle. As chamadas permanecem bloqueadas até o backend ser integrado e validado.


## v0.26 — Modelo de observação (não operacional)

`src/sip_runtime_observation.h` separa o estado do processo e a evidência de eventos SIP de uma mesma sessão. A evidência `VerifiedNativeApi` deve ser emitida apenas por um futuro adaptador nativo de procedência e sessão verificadas. **O executável atual utiliza `SipEvidence::None` e sessão 0**, sem registro, áudio ou chamadas. Nenhum log textual nem processo iniciado é interpretado como registro SIP.

Baresip upstream define `BEVENT_REGISTERING`, `BEVENT_REGISTER_OK`, `BEVENT_REGISTER_FAIL`, `BEVENT_CALL_ESTABLISHED` e `BEVENT_CALL_CLOSED`: https://github.com/baresip/baresip/blob/main/include/baresip.h . Nenhum desses eventos está conectado ao aplicativo ainda. Não habilitar `ctrl_tcp`, `ctrl_http` ou `ctrl_ws` no perfil de laboratório.

Novo CTest valida falha fechada, eventos de sessões antigas, encerramento e controle de chamadas apenas em estados sintéticos com prova nativa. **Testes de política não significam registro SIP real.**


## v0.27 — Contrato de eventos e mailbox worker/UI (não operacional)

Os tipos de evento de src/baresip_event_reducer.h correspondem conceitualmente aos eventos de registro e chamada da API Baresip. O redutor aceita somente eventos de uma fonte previamente marcada como nativa verificada, com sessão atual, processo em execução e sequência válida. Exige Registering antes de RegisterOk, e diferencia Ringing (recebendo), Calling (originando) e InCall (estabelecida). Answer só é permitido enquanto Ringing.

O novo src/baresip_event_mailbox.h aceita um máximo de 32 eventos sem dados pessoais, log textual, URIs ou segredos, protege acesso concorrente worker/UI com mutex, invalida sessões antigas e descarta a sequência inteira após overflow. O frontend Win32 drena periodicamente o mailbox e invalida a sessão no encerramento. **Nesta compilação não há produtor nativo: o mailbox permanece sem sessão e o redutor não recebe evidência VerifiedNativeApi.** Não existe integração, registro ou mídia real.

Gates restantes: fixar/auditar a versão e licenças de Baresip/re; implementar e registrar callback bevent_h nativo, vinculado às identidades de conta/chamada e geração de sessão; tratar falhas, reconexão e ciclo de vida do callback; implementar credenciais protegidas, TLS/SRTP e testes com PABX e headset. A autorização de chamada e de atendimento só poderá ser habilitada após essas validações.

## v0.32 — Projeção persistente e política de recuperação (não operacional)

O novo `src/sip_event_projection.h` conserva o estado SIP confirmado entre ticks da interface e invalida a sessão após perda de evidência, overflow, erro de ordem ou encerramento do processo. O bloqueio é persistente até a criação de uma nova geração.

**O produto ainda não dispõe de produtor nativo Baresip:** a UI passa exclusivamente `SipEvidence::None`, portanto as chamadas permanecem desativadas. Os testes com `VerifiedNativeApi` são sintéticos e validam a política, não uma conta SIP real. Nunca conceder essa evidência a partir de stdout, logs ou simples inicialização do processo.


## v0.33 — identidade de conta e chamada

`BoundBaresipState` exige `account_token` fixado por sessão e `call_token` monotônico, descartando eventos de conta errada, chamadas encerradas ou concorrentes. A projeção do estado na UI exige conta selecionada e rejeita a geração inteira quando a conta é trocada sem reiniciar o motor. Ainda não há fonte nativa: a UI envia `SipEvidence::None` e token 0.

A identidade deve ser emitida exclusivamente por um futuro adaptador Baresip/re versionado. Nunca deve derivar diretamente de texto de stdout, número, URI, cabeçalhos de SIP ou ponteiro após destruição da chamada. **Nenhuma chamada real está habilitada**; CTest verifica apenas regras sintéticas.
## v0.37 — Ingresso condicionado de callbacks nativos (não operacional)

O novo `src/baresip_native_ingress.h` acrescenta uma camada portável de **pré-admissão** de eventos para um futuro adaptador `bevent_h` auditado. Ela exige uma identidade opaca de conta, geração monotônica do mailbox e identidade estável por chamada; rejeita eventos de sessões, contas ou chamadas diferentes. Quando detecta uma divergência ou overflow, **descarta a fila inteira e bloqueia a sessão** até desmontagem explícita.

A API é sincronizada para callbacks de workers e teardown, mas **não autentica os eventos por si mesma**. A operação `bind_validated_account` pressupõe validação nativa da UA e do ciclo de vida por componente ainda inexistente. Eventos alheios ao subconjunto reconhecido não podem criar evidência de registro; não há inclusão de SDK, chamada de rede, armazenamento de dados SIP nem habilitação de `SipEvidence::VerifiedNativeApi`.

O CTest `baresip_native_ingress` usa identidades e eventos *sintéticos* (inclusive replay, mismatch e overflow). O executável Windows ainda utiliza `UnavailableSipEngine`; a camada não é instanciada pela UI. Para ativá-la, são necessários SDK/re versionados e auditados, callback real `bevent_h`, teardown sem corridas, vinculação a UA/call concretos e homologação com PABX/headset. Referência de API para revisão (não é pin de dependência): https://github.com/baresip/baresip/blob/main/include/baresip.h .


**Compatibilidade com o redutor de chamadas v0.36:** o novo ingresso traduz identidades opacas de objetos de chamada em identificadores numéricos monotônicos por geração e propaga `call_id` explícito para o mailbox. Os CTests legados de redutor/projeção agora enviam tokens de chamada sintéticos, acompanhando esse contrato; não habilitam chamadas reais.
