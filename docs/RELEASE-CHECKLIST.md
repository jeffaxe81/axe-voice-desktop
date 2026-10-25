# Release candidata nativa0.53 — checklist atual

## Implementações
- [x] Motor Baresip/libre e OpenSSL fixados, registro/callbacks e comandos chamar/atender/encerrar no candidato.
- [x] Instalador por usuário com DLLs e licenças, teste automático de instalação/desinstalação.
- [x] Microfone e saída escolhidos na aba Áudio aplicados ao registrar; seleção fixa durante a sessão.
- [x] Inventário atualizado antes de iniciar/atender, sem trocar silenciosamente um endpoint explícito ausente; encerrar continua disponível.
- [x] Campos de senha/CA e diagnósticos sem credenciais; validação TLS de CA/hostname.

Os testes Windows/SDK/instalador são exigidos no commit final antes de merge. Consulte a execução do PR correspondente e [escopo0.52](integration/NATIVE-AUDIO-ROUTING.md).

- [x] Mudo/retomada do microfone e espera/retomada local com leitura do SDK e proteção de sessão/chamada (0.53).

## Homologação externa pendente
- [ ] Registrar a conta descartável na central Dígitro identificada.
- [ ] Confirmar chamadas originadas e recebidas entre dois endpoints reais.
- [ ] Confirmar voz nos dois sentidos e headset escolhido; desconexão/reconexão física.
- [ ] Confirmar silêncio do microfone em mudo e espera/retomada aceitas pela central.
- [ ] Confirmar política de certificado, transporte, codecs e eventual exigência de SRTP.
- [ ] Aceitar Windows10/11 físicos e distribuição assinada antes de produção.

Mudo do microfone e espera/retomada locais estão conectados no candidato0.53; transferência continua desabilitada. Espera local não comprova aceite da negociação SIP pela central. A release é candidata até concluir a homologação; não atribuir operação de produção a testes locais.

---

## Checklist histórico da fundação

# AXE Voice v0.16 — release/homologation checklist

## Five incremental software deliveries
1. Human-readable SIP status labels.
2. Declared codec capabilities; G.729 disabled until external support and licensing validation.
3. Fixed diagnostic codes without credentials.
4. Explicit call-action policy (no dialing unless registered).
5. Release gate and validation checklist.

## Strict release gates
- [ ] Windows GitHub Actions build and tests pass for the **final** commit.
- [ ] AXE-Voice.exe opens on Windows 10/11.
- [ ] SIP server and account input validation passes.
- [ ] Baresip pinned version and license inventory approved.
- [ ] Registration passes with a disposable SIP test account.
- [ ] Outgoing and incoming calls confirmed on two endpoints.
- [ ] Bidirectional audio verified with headset.
- [ ] No SIP password or sensitive diagnostic contents stored in logs.
- [ ] TLS/SRTP and codec interoperability tested against target PBX.

Until these are complete, the executable is a prototype GUI, **not** an operational softphone.
