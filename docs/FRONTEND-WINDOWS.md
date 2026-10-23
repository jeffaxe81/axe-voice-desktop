# AXE Voice — Frontend Windows v0.27 (prototipagem)

## Telas

**Áudio (v0.25):** terceira aba para inventário Core Audio do Windows. Mostra nomes de entradas e saídas ativas, marca padrão de comunicação e permite preferência efêmera com fallback após hotplug. **Não escolhe o dispositivo do Baresip** nem prova mídia de chamadas.

**Teclado:** marca AXE Voice, indicação de conta SIP não registrada, campo editável para destino, teclado 12 teclas, comandos Apagar/Limpar e CTA de Chamar explicitamente desativado. Os botões permitem montar um ramal; o campo aceita também entrada manual com URI SIP. Não existe discagem automática.

**Ajustes:** servidor, usuário, porta e seleção TLS (apenas validação local), consulta de dispositivos Core Audio, inspeção dos módulos Baresip e comandos de iniciar/parar motor de laboratório com confirmação. O aplicativo não persiste senha SIP nem efetua registro.

**Aparência:** alternância claro/escuro, opção FIXAR/SOLTAR para manter a janela acima das demais, e MINI/MAX para exibir somente o campo de destino e o status SIP. Abrir Ajustes expande automaticamente a janela. Janela compacta, sem instalador e sem execução em segundo plano. Somente preferências visuais (tema, fixação, modo compacto) são persistidas no Registro do usuário em HKCU\\Software\\AXEVoice\\Desktop; não há armazenamento de credenciais.

**Segundo plano:** ao minimizar, tenta criar um ícone na bandeja de notificações. Duplo clique restaura a janela; o menu oferece Abrir e Sair. Se o shell rejeitar o ícone, não esconde a janela. A saída continua usando encerramento supervisionado do motor.

**Controles de chamada:** botões Chamar, Mudo, Espera, Transferir e Encerrar são visíveis porém indisponíveis, para não simular funções de telefone inexistentes.

**Acessibilidade:** Tab e Shift+Tab navegam pelos campos e botões; Ctrl+K abre o teclado, Ctrl+L foca o destino, Ctrl+, abre Ajustes, Ctrl+T alterna o tema, Ctrl+P fixa/solta; Ctrl+M alterna MINI/MAX. Labels permanecem acessíveis como controles Win32. Leitores de tela e zoom precisam de validação manual.

## Critérios automatizados

- O smoke test da v0.25 também cobre a aba ÁUDIO sem consultar hardware. O teste de política cobre seleção e dispositivo removido.
- O smoke test da v0.24 valida MINI/MAX, restauração automática de Ajustes, dimensões da janela e chamada desabilitada, sem gravar HKCU.
- Teste de modelo de teclado: mapeamento 12 teclas, backspace, rejeição de metacaracteres e limite de caracteres.
- CMake/MSVC e suíte CTest no CI, incluindo `ui_smoke`, que cria a janela de verdade e verifica os controles principais e a troca de abas (sem SIP e sem mostrar a janela).
- Evidência manual ainda necessária: renderização real em Windows 10/11, zoom/DPI, navegação por Tab, compatibilidade com leitores de tela e contraste visual.

## Limites e segurança

Interface gráfica concluída apenas como protótipo. O botão de chamada permanece desativado até haver registro SIP real, circuito de mídia, TLS/SRTP conforme política e homologação. A presença do processo Baresip não deve ser apresentada como registro.

O tema e a opção sempre-no-topo são persistidos exclusivamente no registro HKCU do usuário; não há telemetria, registro SIP nem envio de credenciais.

## Limitações da v0.24

A janela compacta exige revisão manual de escala DPI, leitores de tela e contraste no Windows 10/11. Não indica chamadas ativas: registro SIP e áudio reais continuam fora desta entrega; nenhuma discagem é iniciada ou armazenada.


## Faixa de estado do motor (v0.26)

A faixa superior mostra o estado real do processo Windows (parado, ativo, encerrando ou falha). O estado do processo **não comprova** registro SIP. A UI mantém `UnavailableSipEngine`, e as chamadas não são habilitadas. O CTest `sip_runtime_observation` verifica política de eventos não comprovados e sessões antigas.


## v0.27 — Preparação de eventos nativos

A interface passa a drenar uma caixa de eventos thread-safe do futuro adaptador Baresip a cada ciclo de atualização, mas não tem produtor nem sessão verificada. O banner não interpreta o processo ativo como registro SIP e não habilita chamadas. O modelo inclui Ringing para distinguir chamadas recebidas e autorizar Answer somente nessa condição em testes de política.

## v0.28 — Validação imediata do destino

- Digitação, colagem e teclado de 12 teclas usam o mesmo evento Win32 `EN_CHANGE`.
- Mensagem no rodapé identifica ramal válido, URI SIP válida, espaços, esquema incorreto, domínio/usuário e porta incorreta.
- Campo do destino fica discretamente vermelho quando inválido (também em tema claro); vazio mantém estilo neutro.
- O botão **Chamar** continua bloqueado no protótipo: validade sintática **não** é registro SIP, e uma prova nativa futura não habilita `UnavailableSipEngine`.
- `destination_feedback` testa as classes de retorno com entradas válidas/inválidas; `ui_smoke` exercita o evento `EN_CHANGE` e verifica que nenhuma chamada é habilitada.

**Limites:** sem registro SIP, sessão autenticada, interface de mídia ou chamada real. Teste de cores, DPI e leitores de tela em Windows 10/11 ainda necessário.

## v0.29 — Teclado que respeita cursor e seleção

- As teclas do discador inserem na posição do cursor, substituem a seleção e removem a seleção com **Apagar**.
- O controle Win32 usa `EM_REPLACESEL`, preservando o histórico de desfazer do campo.
- O limite continua em 128 caracteres e os caracteres permitidos do teclado telefônico permanecem os mesmos.
- O teste unitário cobre cursor, seleção, limite e bloqueio de metacaracteres; o smoke Windows exercita cliques reais.
- Nada é discado; registro SIP e áudio bidirecional continuam indisponíveis.

## v0.30 — Validação de conta em tempo real

- Servidor, usuário e porta SIP são inspecionados ao digitar/colar, com erro específico e destaque visual somente em campos inválidos.
- Porta aceita somente dígitos entre 1 e 65535; texto vazio solicita preenchimento sem marcar erro.
- O botão **Validar dados** usa o mesmo modelo puro da validação instantânea, sem configurar conta, enviar credenciais ou registrar no servidor.
- Smoke testa porta inválida/válida, mensagens e bloqueio de chamada. CTest `account_form_feedback` cobre casos extremos.
- O feedback retorna para o destino ao mudar para **Teclado** e indica somente inventário local em **Áudio**.

## v0.31 — Navegação rápida e ajuda offline

- Atalhos adicionais: **Ctrl+1/2/3** alternam entre Teclado/Ajustes/Áudio, **Esc** volta ao Teclado, **F1** mostra ajuda offline, **Ctrl+R** atualiza o inventário somente na aba Áudio.
- Botão **?** abre a mesma ajuda, com instruções que diferenciam validação de sintaxe, processo Baresip e registro SIP real.
- Campos mostram dicas visuais de preenchimento sem gravar conta nem dispositivo. O botão de ajuda permanece disponível em MINI.
- `ui_smoke` confirma a ação de ajuda sem abrir janelas modais, e `ui_help` verifica texto e avisos importantes.
- Pendente homologação manual com acessibilidade, DPI, dispositivos reais e Baresip; não há chamadas reais.

## v0.32 — Banner de estado orientado a sessão

O banner agora consome uma projeção de eventos persistente entre ciclos do timer. Quando não há adaptador nativo validado (situação atual), continua a indicar registro SIP não confirmado. Em um backend futuro, overflow e perda de procedência derrubarão o estado para indisponível até uma nova sessão.

## v0.34 — Diagnóstico de desconexão do headset

- Ao atualizar a aba **Áudio**, as preferências de entrada/saída continuam válidas enquanto o dispositivo existir.
- Quando um headset é removido, a preferência volta ao dispositivo padrão de comunicação do Windows (ou ao primeiro disponível), com aviso específico para microfone, saída ou ambos.
- Quando falta entrada ou saída ativa, o aviso indica o recurso ausente; falhas na consulta Core Audio continuam sendo erros distintos.
- IDs de endpoints são efêmeros e nunca são gravados em HKCU ou exibidos em logs/diagnósticos; mensagem inicial não confunde a primeira escolha com desconexão.
- Novo teste determinístico `audio_selection_feedback` cobre inicialização, retenção, desconexão, falta de dispositivos e não exposição de IDs.
- Nenhuma seleção configura o Baresip; registro SIP, RTP/SRTP, áudio real e chamadas permanecem indisponíveis. Revisão manual de hotplug e headset Windows segue pendente.


## v0.35 — isolamento de sessão em reinício

O processo Baresip não mantém registro SIP presumido após falha, parada ou reinicialização. O novo `SipRuntimeSessionGuard` invalida a fila e a sessão anterior, inclusive ao iniciar um novo processo. Há teste CTest de recuperação. A interface continua sem registro e chamada reais.

## v0.36 — autorização unificada de Chamar

A regra do botão habilitado e a execução de `WM_COMMAND` usam agora a mesma função `authorize_dial`. Ela exige destino válido, sessão SIP atual e autenticada por adaptador nativo, tipo de backend e estado de registro do motor. No clique, a aplicação consulta novamente o processo e a caixa de eventos, evitando confiar apenas na aparência do botão. Testes de política usam apenas um motor simulado; no executável distribuído o `UnavailableSipEngine` continua bloqueando todas as chamadas. Registro, RTP/SRTP e áudio bidirecional reais permanecem pendentes.

## v0.37 — Diagnóstico de prontidão (F2)

- Atalho F2 ou botão na aba Áudio abre um relatório offline de seis requisitos: destino, conta, adaptador nativo, processo, registro verificado e mídia.
- Nenhum usuário, número, senha, domínio SIP ou ID de headset é exibido ou armazenado pelo relatório.
- Inventário de microfones **não** significa mídia RTP/SRTP funcional. No laboratório atual, a verificação de mídia permanece bloqueada.
- Smoke test cobre o comando e a impossibilidade de habilitar Chamadas. O novo CTest valida todos os gates isoladamente.
- O relatório não altera a política de chamada nem substitui testes em PBX real.

## v0.40 — Acessibilidade: alto contraste nativo do Windows

- A UI consulta o modo **Alto Contraste** do Windows ao iniciar e nos eventos de mudança de configuração, cores do sistema ou tema.
- Quando ativo, fundos, textos, bordas, campos e botões usam `GetSysColor` em vez das cores da identidade visual. A preferência claro/escuro permanece salva, mas **não se sobrepõe** ao modo de acessibilidade.
- Controles desabilitados seguem usando a cor de texto desabilitado do Windows; abas selecionadas utilizam Highlight/HighlightText. Alertas de campo inválido continuam descritos em texto, sem depender exclusivamente de cor.
- Um teste puro de política verifica cada papel de cor e sua independência de claro/escuro no modo de alto contraste. O teste Win32 `ui_smoke` verifica o mapeamento e mantém o bloqueio de chamadas.
- Não altera credenciais, registros, telefonia ou preferências HKCU ao receber uma mudança de acessibilidade.

A validação de contraste percebido, leitores de tela e escala DPI **ainda exige testes manuais em Windows 10/11**. 


## v0.42 — DPI por monitor

- Solicita Per-Monitor DPI Awareness V2 no Windows 10/11.
- Redesenha todos os controles e fontes a partir do layout lógico original de 96 DPI, prevenindo deriva entre mudanças repetidas de escala.
- Cartões, teclado telefônico, indicadores e botões owner-draw acompanham o DPI. MINI/MAX preserva proporções, incluindo 150% e 200%.
- O smoke test Win32 verifica coordenadas do campo, troca de página e MINI nas escalas 150% e 200%, sem alterar as configurações reais do Windows. Novo teste unitário cobre a matemática do escalonamento.
- Seguem pendentes os testes manuais em monitores físicos de DPI distintos, leitores de tela, truncamento de textos e resoluções pequenas. Registro SIP e áudio de chamada continuam não homologados.

## v0.42 — Consolidação: alto contraste, headset e áudio assíncrono

- A aba Áudio lista headsets de forma automática a cada 3 segundos, em uma thread de trabalho, sem bloquear a janela durante consultas Core Audio.
- Preserva escolha explícita do usuário em desconexões, realiza fallback temporário e restaura o headset preferido quando ele retorna.
- Alterações de abas invalidam resultados atrasados; combos abertos não são reconstruídos. Erro de dispositivo oculta listas inválidas, preservando preferência em memória.
- Alto Contraste do Windows mantém a precedência sobre o tema claro/escuro, inclusive em controles desabilitados.
- Atualização manual encerra o estado "Consultando..." mesmo se o catálogo estiver inalterado.
- Novos testes: `audio_hotplug_refresh`, `audio_preference_state` e `audio_inventory_async`. `ui_smoke` não acessa hardware e mantém o botão Chamar bloqueado.
- **Limitações:** botão de diagnóstico em Ajustes ainda consulta Core Audio de forma síncrona; áudio Baresip, RTP/SRTP e SIP reais não são implementados. Exige validação em Windows 10/11 com headset/driver e leitores de tela.

## v0.43 — Diagnóstico de áudio não bloqueante na aba Ajustes

- O botão **DISPOSITIVOS DE ÁUDIO**, em Ajustes, deixa de executar Core Audio na thread da janela.
- Um worker dedicado apresenta "Consultando..." e entrega apenas o resumo local de microfones/saídas quando a enumeração conclui. A aba Áudio continua com seu próprio worker e hotplug.
- Pedidos repetidos são coalescidos; sair de Ajustes descarta o resultado pendente, inclusive se o driver responder após a navegação.
- Uma nova consulta invalida resultados anteriores não consumidos, evitando exibir um diagnóstico obsoleto.
- O teste `ui_smoke` exercita o botão sem enumerar hardware. O CTest `audio_inventory_async` cobre resultado antigo descartado, reconsulta e falhas.
- O relatório continua sem credenciais, números SIP ou IDs de headset, e **não confirma áudio RTP/SRTP real**. Testes manuais Windows/driver permanecem necessários.

## v0.44 — Headset: reação acelerada a eventos do Windows

- A aba Áudio recebe `WM_DEVICECHANGE` para mudanças da árvore de dispositivos, chegada, remoção e reconfiguração. O sinal agenda nova **consulta assíncrona** após 250 ms, em vez de esperar até 3 segundos.
- Eventos sucessivos são agrupados por debounce; se o worker estiver ocupado, a consulta nova é tentada após ele liberar, sem atualizar combos diretamente da callback Win32.
- Um snapshot de antes do evento é invalidado; a escolha de headset e seu fallback/restauração permanecem protegidos pela política v0.39.
- Sair da aba ou fechar a janela cancela o timer e invalida resultados atrasados. O teste smoke não consulta hardware.
- A verificação periódica de 3 segundos permanece **obrigatória**: `WM_DEVICECHANGE` é só um indício e não cobre necessariamente mudanças do dispositivo padrão.
- O CTest `audio_device_change_policy` verifica categorias válidas, filtros de página/CI smoke e descarte de eventos não aplicáveis.
- **Limites:** esta entrega não prova headset real, permissão de microfone, ligação Baresip, registro SIP ou RTP/SRTP. Os testes com USB/Bluetooth físicos e Windows 10/11 continuam pendentes.

## v0.45 — Atualização manual confiável com headset ocupado

- Quando o usuário clica em **Atualizar dispositivos** e a enumeração do Windows ainda está ocupada, a solicitação fica pendente e o snapshot antigo é invalidado.
- Vários cliques durante a mesma consulta geram **somente uma reconsulta** quando o worker liberar, sem executar Core Audio na thread Win32.
- A interface mostra que a atualização aguarda a consulta anterior; o timer de 100 ms inicia a reconsulta quando possível, antes de exibir um inventário atualizado.
- A mudança de aba ou encerramento cancela pedidos pendentes; nenhum ID de headset é persistido, exibido no status ou entregue ao Baresip.
- Novo CTest `audio_manual_refresh` cobre coalescência, cancelamento, worker lento e descarte de resultado obsoleto.
- Ainda não há configuração de mídia Baresip nem teste real de headset/USB/Bluetooth; testes manuais Windows 10/11 permanecem obrigatórios.
