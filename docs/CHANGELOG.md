## 0.39.0 — 2026-10-22 (laboratório)

- Retorna à preferência explícita de headset quando o dispositivo reaparece, sem persistir IDs.
- Preserva seleção em memória em erros temporários do Core Audio e impede que a UI exiba inventário inválido como ativo.
- Novo teste determinístico de preferências de áudio; registro/chamadas SIP permanecem desabilitados.

## 0.38.0 — 2026-10-22 (laboratório)

- Atualização automática do catálogo Core Audio a cada 3 segundos na aba Áudio, sem redefinir seleções quando nada mudou.
- Teste de regressão para hotplug e padrões de comunicação; integração SIP e áudio real continuam pendentes.

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
