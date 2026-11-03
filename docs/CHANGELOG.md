# Changelog

## v0.82.0 — idempotência CTI reforçada e matriz de 1.400 cenários (candidata)
- Reutilização de `Idempotency-Key` com destino normalizado diferente retorna HTTP 409 `idempotency_key_conflict`, sem nova chamada e sem divulgar o número original; replay válido permanece com o mesmo ID.
- Intenção original do pedido vinculada à chave **apenas na memória privada** e removida pelo limite de 32 resultados ou ao reiniciar.
- Novo teste de estresse C++ com **1.400 cenários determinísticos** (7 domínios × 200), incluindo **800 threads** de disputa, privacidade, histórico e eventos nativos simulados.
- Loopback HTTP com **120 transações adversariais** adicionais para rotas, Bearer e Origin. A matriz é executada mais 3 vezes no gate, para 4.200 cenários de repetição.
- CI nativa e de interface preservada; não representa 1.400 chamadas reais, cobertura total de código nem homologação física SIP/áudio.
- Guia: `docs/quality/CTI-1400-STRESS-MATRIX.md`, contrato atualizado em `docs/integration/LOCAL-CTI-IDEMPOTENCY.md`.

## v0.81.0 — descoberta da API CTI e exemplo para integração (candidata)
- Novo `GET /v1/capabilities` autenticado em loopback, com funcionalidades realmente implementadas, limites de histórico e semântica de submissão de chamadas.
- Explicitamente `sip_request_correlation=false` e `call_completion_confirmation=false` para não confundir request_id com prova de chamada atendida.
- Segurança existente preservada: Bearer obrigatório, Host local exato, sem CORS ou webhooks; testes unitários e HTTP de bloqueio de Origin, Referer, corpo e métodos inválidos.
- Novo exemplo PowerShell de descoberta de recursos, discagem idempotente, consulta de solicitação, eventos por ID e cancelamento apenas na fila.
- Documentação: `docs/integration/LOCAL-CTI-CAPABILITIES.md` e `examples/cti/README.md`.
- Contrato CTI de laboratório, ainda sem homologação de chamadas, mídia ou integração externa.

## v0.80.0 — estabilidade do gate de integração libre (candidata)
- `libre_owner_reactor` passa a aguardar eventos reais de tick/falha sob prazo máximo de 1 s, sem depender de sleeps fixos de 20–30 ms.
- Falhas passam a indicar código de cenário e contadores nativos, facilitando investigação da issue #169.
- O gate de integração executa mais 10 repetições fail-fast do teste de reator nativo; qualquer falha continua reprovando a CI.
- Nenhuma mudança no motor SIP/áudio de produção; causa raiz da intermitência anterior não declarada resolvida.
- Detalhes em `docs/integration/LIBRE-REACTOR-TEST-STABILITY.md`.

## v0.79.0 — eventos CTI filtrados por solicitação (candidata)
- Novo `GET /v1/requests/{id}/events?after=N`: consulta autenticada de transições pertencentes somente ao pedido indicado.
- Filtragem e verificação de ID conhecido no mesmo mutex; sem vazamento de eventos de outros pedidos, destinos, credenciais ou IDs nativos.
- Preserva cursores globais, histórico de 64 eventos e `gap` conservador; IDs retirados do histórico de 32 resultados retornam 404.
- Novos testes unitários e HTTP de isolamento, autenticação, cursores, cancelamento e eventos expirados.
- Guia: `docs/integration/LOCAL-CTI-SCOPED-EVENTS.md`; sem homologação física com central/áudio.

## v0.78.0 — pré-desinstalação com verificação segura (candidata)
- Teste de empacotamento nativo aguarda no máximo 15 segundos pela ausência dos marcadores de aplicação/setup e acesso exclusivo ao executável antes de desinstalar.
- Bloqueios e erros inesperados permanecem reprovatórios; nenhum processo é encerrado e nenhuma proteção do Inno Setup é desabilitada.
- Falha da desinstalação silenciosa passa a informar código de saída e últimas 40 linhas do log do Inno para diagnóstico.
- Novo teste real de mutexes, arquivo bloqueado/inexistente e espera limitada; acompanha a intermitência da issue #166, cuja causa segue não confirmada.
- Detalhes em `docs/integration/NATIVE-UNINSTALL-READINESS.md`.

## v0.77.0 — eventos por solicitação CTI (candidata)
- Novo `GET /v1/request-events?after=N`: polling autenticado de transições da solicitação com `request_id` e sequência incremental.
- Emissão atômica de estados `queued`, `dispatching`, `submitted`, `rejected` e `cancelled`; retries idempotentes não geram eventos duplicados.
- Histórico de até 64 transições, indicador `gap` ao perder eventos e bloqueio de acesso externo/CORS mantido.
- Eventos de pedidos não representam confirmação de chamada SIP atendida ou RTP; sem correlação inferida de call_id.
- Cobertura de testes da fila, HTTP local, isolamento e cursor; documentação em `docs/integration/LOCAL-CTI-REQUEST-EVENTS.md`.

## v0.76.0 — idempotência opcional do Click-to-Call (candidata)
- Novo cabeçalho `Idempotency-Key` opcional em `POST /v1/calls`; chaves válidas têm 16–64 caracteres ASCII seguros.
- Chave repetida retorna HTTP 200 com mesmo `request_id`, status existente e `replayed=true`, sem nova chamada, mesmo se a aptidão SIP mudou.
- Chaves retidas apenas no histórico efêmero de 32 resultados; não fornece idempotência entre reinícios.
- Testes de parser, estado da fila, cancelamento, submissão, diferença de destino e autorização HTTP.
- Documentação: `docs/integration/LOCAL-CTI-IDEMPOTENCY.md`; ainda sem homologação em PABX real.

## v0.75.0 — cancelamento de pedidos CTI enfileirados (candidata)
- Novo `DELETE /v1/requests/{id}` autenticado, capaz de cancelar atomicamente uma solicitação ainda na fila.
- Novo status `cancelled` na consulta de resultados da v0.74.
- Rejeita cancelamento depois de retirada para execução com 409; não envia SIP BYE/CANCEL nem tenta encerrar chamada estabelecida.
- Mesma proteção de mutex para cancelamento e retirada de fila; testes de concorrência lógica, HTTP e privacidade.
- Documentação: `docs/integration/LOCAL-CTI-CANCEL-QUEUED.md`. Sem homologação com central real.

## v0.74.0 — rastreamento de solicitações CTI (candidata)
- `POST /v1/calls` retorna um `request_id` efêmero com o status `queued`.
- `GET /v1/requests/{id}` permite consultar a tramitação: `queued`, `dispatching`, `submitted` ou `rejected`.
- Fila CTI continua exclusiva até a confirmação de envio ao motor SIP; não libera o slot quando a GUI apenas retira o pedido da fila.
- Rotina de envio SIP compartilhada com o botão CHAMAR. `submitted` indica apenas retorno do motor, nunca chamada conectada.
- No máximo 32 resultados transitórios, sem telefones ou identificadores internos no retorno.
- Adicionados testes unitários e HTTP de privacidade, autorização, fila e resultados. Ver `docs/integration/LOCAL-CTI-REQUEST-TRACKING.md`.

## v0.73.0 — acompanhamento CTI de chamadas (candidata)
- API local acrescenta `GET /v1/calls/current` e `GET /v1/events?after=N` com autenticação existente.
- Estado derivado somente de observação SIP nativa verificada; estados não comprovados aparecem como `unverified`.
- Histórico de até 32 transições em memória, com números sequenciais e indicador de eventos perdidos (`gap`), sem destinos, credenciais ou IDs nativos expostos.
- Testes de políticas de sessão, regressão HTTP, transições, paginação, privacidade e autorização.
- Escopo detalhado em `docs/integration/LOCAL-CTI-STATUS-EVENTS.md`. Sem webhooks e sem homologação em central/headset real.

## v0.72.0 — Click-to-Call e API CTI local (candidata)
- Link `axvoice://call?destination=...` registrado pelo instalador do candidato nativo. O link é validado, redirecionado à janela existente e exige confirmação explícita do operador antes de discar.
- API HTTP com token, opt-in, restrita a `127.0.0.1:27652`: `GET /v1/status` e `POST /v1/calls`.
- Solicitações CTI entram em fila de uma posição e são novamente autorizadas no thread da GUI pelo mesmo controle do botão CHAMAR; nenhum bypass de SIP.
- Sem CORS ou acesso remoto; HTTP não deve ser chamado diretamente por JS em páginas de CRM. Contrato: `docs/integration/LOCAL-CTI-CLICK-TO-CALL.md`.
- Testes de parsing, segurança HTTP, autenticação e fila local. Homologação em central real, browser e headset ainda pendente.

## v0.71.0 — diagnóstico efêmero de microfone no Windows (candidata)
- Botão **MICROFONE** na aba ÁUDIO, disponível apenas com dispositivo verificado no inventário e acionado manualmente.
- Captura WASAPI em memória por ~850 ms, indicador de presença/ausência de sinal e acesso/privacidade; nenhuma gravação ou transmissão.
- Intertravamento com prévias de voz/toque, chamadas/alertas e troca de páginas; seleção explícita desconectada nunca usa fallback.
- Testes determinísticos para seleção, formatos de amostra, buffers inválidos, NaN e smoke visual. Evidência em `docs/integration/LOCAL-MICROPHONE-TEST.md`.
- Não comprova mídia SIP/RTP bidirecional nem homologação em headset físico ou central Dígitro.

## v0.70.0 — teste local independente da saída de voz (candidata)
- Botão **TESTAR VOZ** na aba Áudio, separado de **TESTAR TOQUE**, com sinal de 250 ms enviado ao dispositivo de conversação escolhido via WASAPI.
- O padrão de comunicação do Windows é resolvido pelo inventário atual; saída escolhida e desconectada bloqueia a reprodução sem fallback silencioso.
- Um único player evita sobreposição; teste indisponível durante chamada ativa, toque recebido, inventário pendente ou ausência de saída.
- Novo teste determinístico da seleção de dispositivos e atualização do teste de interface. **Não comprova voz bidirecional**, PABX ou headset físico.
- Procedimento e limitações: `docs/integration/CONVERSATION-OUTPUT-TEST.md`.


## v0.51.0 — watchdog de comandos SIP pendentes (laboratório)
- `SipCommandExecutor` exige relógio monotônico e bloqueia novas operações após 10 segundos sem confirmação de mudança de estado.
- `watchdog()` permite detectar timeout mesmo quando o usuário não envia novos comandos; falha de prova SIP e retrocesso do relógio também invalidam a sessão.
- Nova suíte de testes sintéticos de timeout, progresso, reinício e impossibilidade de repetir comandos antigos; integração real ainda não habilitada na GUI.
- Documento `docs/SIP-COMMAND-WATCHDOG.md` registra comportamento, limites e integração futura.


## v0.50.0 — teste de toque no dispositivo Windows selecionado
- Botão explícito TESTAR TOQUE na aba ÁUDIO. Renderiza sinal local de 660 Hz com volume baixo e duração limitada via WASAPI na saída de toque escolhida, em thread separada.
- Escolha da saída de conversa e do microfone permanece independente; não há roteamento SIP/RTP ou reprodução de chamadas reais.
- Teste unitário de formatos PCM16/PCM32/Float32, limites de buffer, envelope de volume e dispositivo inválido, além de validação de presença e desabilitação do botão no teste de interface sem hardware.
- A confirmação física do áudio e testes de hotplug no computador permanecem como homologação manual.


## v0.49.0 — posse exclusiva do callback Baresip (laboratório)
- Impede que duas instâncias do adaptador disputem o registro global de eventos da API Baresip.
- A posse permanece protegida durante falha e encerramento; só é liberada depois de desregistrar e aguardar callbacks.
- Teste com registro SDK simulado verifica concorrência lógica, tentativas indevidas, recuperação e 64 reinícios alternados.
- Continua sem Baresip real, credenciais, chamadas SIP ou mídia integrada no aplicativo Windows.
- Adicionada pré-visualização de toque independente da saída de voz na aba ÁUDIO: padrão multimídia para toque, comunicação para voz, seleção inversa e fallback de hotplug, sem áudio SIP efetivo.


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
