# AXE Voice LAB — v0.140–v0.144: observação de dispositivos e proteção de callbacks

Data da rodada: 24/10/2026. **Compilação Windows, execução de testes e PABX não realizados por solicitação do usuário.**

## Reconciliação seletiva dos rascunhos

| Versão | Origem histórica | Implementação na branch v0.144 |
| --- | --- | --- |
| 0.140 | PR #30 | `AudioHotplugPolicy` reconhece também aviso genérico `DBT_CONFIGCHANGED` (0x18), além de remoção/chegada/mudança de nós. Não confere permissões nem prova áudio. |
| 0.141 | PR #40 | Integração de `AudioEndpointWatcher` e `AudioEndpointNotificationSink`, baseada em `IMMNotificationClient`, com cadastro COM opcional e notificação por `PostMessageW` sem enviar identificador de headset, usuário SIP ou ponteiros de sessão. |
| 0.142 | PR #40 | Notificações também para mudanças de padrão eMultimedia (toque) e eCommunications (voz). Inventário e reconfiguração não são executados nos callbacks COM. |
| 0.143 | PR #40 | `AXEVoice.exe` registra o observador após criar a janela, agrega avisos de áudio no debounce existente, atualiza somente quando a aba Áudio estiver visível e libera observador ao destruir a janela. Fallback para WM_DEVICECHANGE e refresh manual se registro COM falhar. |
| 0.144 | PR #31 + versão | `BaresipCallbackLifecycle::hold_native_callback` confere tanto epoch/generation como `ingress_.session()` capturada antes de entregar o lease de callback. CMake, título da janela e diagnóstico local passam a v0.144. |

## Escopo e segurança

- Core Audio apenas informa mudanças de inventário, sem acessar microfone, decodificador, senha ou tráfego SIP no callback.
- A GUI mantém `UnavailableSipEngine` com `SipEvidence::None`. Dispositivos de áudio locais **não** provam que há RTP/SRTP em chamada.
- O módulo de observação é opcional. O tratamento de `WM_DEVICECHANGE` e botão de atualização manual permanecem.
- `UnregisterEndpointNotificationCallback` é solicitado antes de destruir o HWND; um callback já em execução pode ter postado uma mensagem antiga. A mensagem contém apenas um tipo fixo e não é uma instrução SIP.
- O SDK Baresip experimental continua desligado por padrão e segregado do aplicativo de produção. A proteção de sessão não autentica por si só a procedência de eventos.
- A implementação **não** incorpora automaticamente as demais mudanças antigas de #30/#31/#40, por exemplo a varredura assíncrona de áudio, a suíte de testes ou SDK de procedência auditada.
- É necessária posterior compilação MSVC, teste de reconexão no Windows 10/11 com headset e alto-falante, alternância de saída multimídia, teardown durante callbacks e auditoria de ABI/SDK nativa.

## Evidências desta rodada

Somente comparação de código e presença de barreiras estáticas. Não foi executado binário, teste unitário, CTest, CI manual ou homologação com PABX; não alegar aprovação da implementação.

A `main` continua em arquitetura histórica, recebendo no máximo rastreabilidade documental até promoção autorizada de código funcional.
