# Changelog

## v0.48.0 — isolamento de origem dos eventos Baresip (laboratório)
- `BEVENT_SIPSESS_FAILED/CONN` são eventos de mensagem SIP, não evidência de falha da UA selecionada.
- Allowlist valida o tipo nativo antes de consultar `bevent_get_ua`; eventos desconhecidos, de RTP e criptografia são ignorados sem comprovar mídia segura.
- Testes simulados cobrem ausência de alteração de estado após eventos sem vínculo confiável com a UA.
- Sem registro SIP e chamadas reais no executável.


## v0.47.0 — executor SIP com isolamento de comandos repetidos (laboratório)
- Executor opcional de comandos originar/atender/desligar, usado somente no teste, preparado para worker SIP nativo serializado.
- Revalida estado, geração e ID opaco da chamada; rejeita sequências duplicadas, ações repetidas e comandos antes de transição SIP.
- Falhas e exceções na chamada ao port nativo isolam a sessão, impedindo nova tentativa com resultado incerto.
- Teste simulado de 100 tentativas repetidas; GUI segue sem registro SIP, chamadas ou RTP/SRTP.


## v0.46.0 — link/ABI smoke test do SDK Baresip (laboratório)
- Opcional `axv_native_baresip_sdk_abi_probe` verifica, no link de um executável, símbolos do SDK real aprovado pelo gate SHA256. É build-only e exige o SDK real fornecido externamente.
- `baresip_sdk_abi_probe` exercita no CI a verificação com SDK simulado; não atesta compatibilidade com Baresip real.
- CMake mantém `AXV_BUILD_NATIVE_BARESIP_ADAPTER=OFF` por padrão; a GUI continua sem SIP/RTP/SRTP, chamadas ou mídia reais.


## v0.45.0 — autorização de atender e desligar por sessão/chamada
- Novo controle puro de comandos Dial/Answer/Hangup com sessão SIP válida e identificador opaco de chamada.
- Impede atender/desligar chamadas já encerradas, identificadores de outra chamada, divergência de estado entre backend e UI, e discagem com chamada ativa.
- Teste de integração sintética registra/recebe/estabelece/encerra/chama novamente, e nega 100 tokens antigos.
- O softphone continua com `UnavailableSipEngine`: não executa chamadas, atendimentos nem áudio reais.


## v0.44.0 — isolamento de gerações do adaptador SDK (laboratório)
- Callback nativo captura geração e sessão imutáveis; evento antigo não contamina reinício.
- `BaresipCallbackLifecycle` agora gerencia revogação e espera no adaptador isolado.
- Falha dentro de callback revoga sem auto-bloqueio; teste adicional cobre 100 reinícios.
- Sem integração SIP, chamadas ou mídia reais no executável AXE Voice.

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
