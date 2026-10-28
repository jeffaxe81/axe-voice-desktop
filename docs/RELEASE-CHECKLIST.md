# Release candidata nativa0.65 — checklist atual

## Implementações
- [x] Motor Baresip/libre e OpenSSL fixados, registro/callbacks e comandos chamar/atender/encerrar no candidato.
- [x] Instalador por usuário com DLLs e licenças, teste automático de instalação/desinstalação.
- [x] Microfone e saída escolhidos na aba Áudio aplicados ao registrar; seleção fixa durante a sessão.
- [x] Inventário atualizado antes de iniciar/atender, sem trocar silenciosamente um endpoint explícito ausente; encerrar continua disponível.
- [x] Campos de senha/CA e diagnósticos sem credenciais; validação TLS de CA/hostname.

Os testes Windows/SDK/instalador são exigidos no commit final antes de merge. Consulte a execução do PR correspondente e [escopo0.52](integration/NATIVE-AUDIO-ROUTING.md).

- [x] Mudo/retomada do microfone e espera/retomada local com leitura do SDK e proteção de sessão/chamada (0.53).

- [x] Limpeza do aviso de áudio ao encerrar/trocar chamada e pedido de transferência direta com revisão de destino e identidade da chamada (0.54).

- [x] Toque de chamada recebida verificada, saída independente, cancelamento e ações vinculadas à chamada exibida (0.55).

- [x] COPIAR STATUS / Ctrl+Shift+D: relatório local de indicadores sem dados da conta, com cópia Unicode e tratamento de falhas (0.56).

- [x] DTMF via SIP INFO com modo explícito, validação da chamada e recusa visível de fila ocupada (0.57).

- [x] Recuperação após falha de criação do toque, foco de botões com rolagem e pacote vinculado ao executável testado (0.57.1).

- [x] Duração local condicionada à evidência da chamada, consistência de versões e regressão de handshake TLS (0.58).

- [x] Consultas de dispositivos fora da GUI, canais independentes e descarte de gerações obsoletas (0.59).

- [x] Observação opcional de dispositivos, debounce/fallback sem inanição e callbacks sem acesso à UI (0.60).

## Homologação externa pendente

**Mantida pendente por solicitação do usuário em 25/10/2026.** O desenvolvimento continua; nenhum item de central/headset está aprovado por essa decisão.

- [ ] Validar copiar/colar, atalho e leitor de tela em Windows físico.
- [ ] Registrar a conta descartável na central Dígitro identificada.
- [ ] Confirmar chamadas originadas e recebidas entre dois endpoints reais.
- [ ] Confirmar audibilidade do toque, parada ao atender/recusar e desconexão/reconexão da saída de toque no Windows físico.
- [ ] Confirmar voz nos dois sentidos e headset escolhido; desconexão/reconexão física.
- [ ] Confirmar silêncio do microfone em mudo e espera/retomada aceitas pela central.
- [ ] Confirmar recebimento SIP INFO/DTMF e navegação da URA na central.
- [ ] Confirmar REFER/NOTIFY, destino final e áudio após transferência direta na central.
- [ ] Confirmar política de certificado, transporte, codecs e eventual exigência de SRTP.
- [ ] Aceitar Windows10/11 físicos e distribuição assinada antes de produção.

Mudo, espera/retomada locais e pedido de transferência direta estão conectados no candidato0.54. Transferência com consulta permanece indisponível. Espera local não comprova aceite da negociação SIP pela central. A release é candidata até concluir a homologação; não atribuir operação de produção a testes locais.

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

Patch0.55.1: o relatório F2 mostra a disponibilidade atual de Chamar separada da homologação de áudio. Checklist pendente não significa bloqueio de discagem; botão disponível não comprova mídia ou compatibilidade da central.

- Candidate0.61: exigir regressões de segurança de entradas/credenciais/autorização e teste PowerShell de hash literal, além dos gates Windows/SDK/instalador do commit final. [Limites e acompanhamento](integration/SECURITY-HARDENING.md).

- Candidate0.62: verificar política do motor independente do checkbox, metadados limpos ao parar, falhas init/queue sem senha pendente, hosts malformados recusados e diretório SDK redirecionado bloqueado. [Escopo e testes](integration/SECURITY-STATUS.md).

- Candidate0.63: exigir motivos de destino e reset, cinco modos nativos de ciclo/credencial, schema de manifesto estrito e isolamento da exportação. [Escopo e limites](integration/SECURITY-FEEDBACK.md).

- Candidate0.64: exigir matriz de falhas de configuração inicial/substituição, ciclo de diagnóstico, perfil Windows com separador final e arquivos SDK/licenças extras recusados. [Evidência e limites](integration/SECURITY-RECOVERY.md).

- Candidate0.65: exigir callbacks de limpeza no proprietário, primeira falha preservada, rejeições de conta sem registro antigo e nomes de manifesto recusados antes de hash. [Escopo](integration/SECURITY-OWNER-LIFECYCLE.md).
