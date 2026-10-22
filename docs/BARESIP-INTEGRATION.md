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
