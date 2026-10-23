## v0.47.0 — 2026-10-22 (laboratório)

- Adiciona observador nativo de alterações de endpoints Core Audio, inclusive dispositivo padrão de comunicação.
- Callback COM sinaliza a UI, sem enumerar no evento; resultado é tratado pelo worker assíncrono já existente.
- Teste offline de callback, detach e ciclo de referência. Sem áudio SIP real.

## v0.45.0 — 2026-10-22 (laboratório)

- Pedidos manuais de atualização de áudio deixam de ser perdidos quando o Core Audio já está ocupado.
- Reconsultas são coalescidas e resultados anteriores invalidados; novos testes sintéticos de worker ocupado.
- Sem registro SIP e mídia real: UI permanece fail-closed.

## v0.44.0 — 2026-10-22 (laboratório)

- Eventos Windows de conexão/remoção passam a antecipar a consulta assíncrona de headset com debounce.
- Snapshots anteriores ao evento são descartados e a atualização periódica é mantida como fallback.
- Novo teste determinístico `audio_device_change_policy`; sem SIP ou mídia real.

## v0.43.0 — 2026-10-22 — diagnóstico local de áudio (laboratório)

- Consulta de dispositivos em Ajustes movida para um worker independente, sem bloquear a UI.
- Resultado obsoleto removido ao aceitar nova consulta; troca de aba cancela atualização visual.
- Smoke Windows e regressões de reconsulta no CTest.
- Nenhum registro SIP, chamada ou áudio de Baresip real ativado.

## v0.42.0 — 2026-10-22 (laboratório)

- Alto contraste v0.40 combinado com hotplug, preferências restauráveis e consulta assíncrona de headsets.
- Três testes adicionais, preservando políticas de autorização SIP fail-closed.
- Sem chamadas, registro SIP ou mídia RTP/SRTP reais.

## v0.41.0 — isolamento de eventos de terminal e falhas SIP (laboratório)
- Corrigido `BEVENT_SHUTDOWN`: somente eventos da UA selecionada encerram a sessão; `BEVENT_EXIT` permanece global.
- `BEVENT_AUDIO_ERROR` e `BEVENT_SIPSESS_FAILED` da UA selecionada invalidam evidências pendentes e a sessão do bridge.
- Teste com SDK simulado ampliado para outras UAs, falhas de áudio/SIP, encerramento global e reinício.
- Adaptador continua isolado da GUI, sem conta registrada ou chamadas reais.

# Changelog

## v0.37.0 — prontidão de chamada offline
- Checklist com seis bloqueadores acessível por F2 e na aba Áudio.
- Dados de conta e destino nunca aparecem no relatório; somente status locais.
- Novos testes determinísticos de gates e smoke de interface.
- Continuam ausentes integração Baresip in-process, registro SIP e mídia real.

## v0.36.0 — controle único de autorização de discagem (protótipo)
- Botão Chamar e comando Win32 direto passam pela mesma verificação: destino válido, geração SIP atual, prova nativa auditada, backend Baresip e status registrado.
- Clique consulta novamente o estado real do processo; evento SIP antigo e botão ativado indevidamente não autorizam chamada.
- Testes sintéticos exercitam tentativas negadas, chamada autorizada somente no mock e normalização de destino. O aplicativo real continua com UnavailableSipEngine; sem chamada RTP/SRTP ou registro SIP real.

# Changelog

## v0.35.0 — isolamento de sessões SIP em reinício (protótipo)
- Guarda de ciclo de vida descarta eventos e prova SIP de processos anteriores ao iniciar, parar, falhar ou reiniciar.
- Estado de sessão persiste apenas em memória entre ticks do mesmo processo; nunca é restaurado do disco.
- Aplicação continua usando `SipEvidence::None`, sem adaptador Baresip ou chamadas habilitadas.
- Teste de recuperação, queda, parada voluntária e callbacks obsoletos.

## 0.34.0 — 2026-10-22 (laboratório)

- Aba Áudio informa desconexão e fallback de microfone/saída sem registrar dados do dispositivo.
- Novo teste determinístico do diagnóstico de hotplug; chamadas SIP permanecem bloqueadas.

# Changelog

## v0.32.0 — projeção SIP persistente (protótipo)
- Estado SIP é mantido entre ciclos de atualização, condicionado a eventos nativos validados de uma única geração.
- Falha fechada após desordem de eventos, overflow de fila, perda de prova, término do motor; exige nova geração para recuperação.
- Front-end continua sem adaptador Baresip real, usando `SipEvidence::None`; sem registro ou chamadas reais.
- Novo teste determinístico de projeção.


## v0.28.0 — feedback de discagem em tempo real (protótipo)
- Campo de destino com validação na digitação, colagem e teclado numérico via EN_CHANGE.
- Feedback semântico de erros e aviso de bloqueio SIP; destaque suave para destino inválido.
- Guarda dupla: somente engine SIP utilizável, evento nativo comprovado e destino válido podem habilitar chamada em uma versão futura. Sem backend Baresip integrado.
- Novo teste determinístico de feedback e mais cenários no smoke da janela.


## v0.19.0 — parada responsiva do motor (laboratório)
- Solicitação `request_quit()` separada da espera pelo encerramento.
- Timer da janela supervisiona o processo a cada 100 ms, aplica 1,5 s de tolerância e desabilita start/stop durante parada.
- Novo teste de política de timeout determinístico e casos de requisição única ao filho de testes.
- Não altera registro SIP, chamadas nem mídia; Baresip real ainda exige homologação.

## Unreleased — desenvolvimento v0.16.2
### Added
- Inventário real de microfones e saídas ativos no Windows (MMDevice/Core Audio) com padrões de comunicação.
- Botão de diagnóstico de áudio no protótipo Win32 e teste determinístico sem dependência de hardware.
- Script scripts/build-windows.ps1 para configurar, compilar e testar no Windows.
- CI passa a executar o mesmo script; artifact continua restrito ao protótipo de interface.
### Not available
- Motor Baresip, registro SIP, chamadas RTP/SRTP e áudio bidirecional; homologação manual de headset pendente.

### Added
- Fundação inicial do projeto.
- README principal.
- Estrutura documental.
- Estratégia inicial de branches.
