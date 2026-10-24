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

## v0.47 — Notificações oficiais do Core Audio

A aba ÁUDIO passa a receber eventos do `IMMNotificationClient` para troca do dispositivo padrão de comunicações, inserção/remoção, estado ou propriedades de endpoint. O callback COM apenas agenda uma mensagem sem IDs para a fila da janela; o inventário acontece após debounce de 400 ms no loop Win32. A subscrição é liberada no `WM_DESTROY`, antes de destruir a janela. Se COM não puder inicializar, a atualização manual e a sinalização `WM_DEVICECHANGE` continuam ativas. O teste de callback usa janela message-only e eventos sintéticos, sem hardware. O Baresip ainda não recebe seleção de dispositivos nem realiza chamadas.

## v0.48 — Seletor duplo de toque e voz

A aba ÁUDIO possui 3 seletores: microfone, saída para **voz remota** e saída para **toque da chamada**. A ação INVERTER TOQUE/VOZ troca apenas as duas saídas, sem tocar o microfone. Exemplos: computador toca e headset reproduz a voz, ou headset toca e computador reproduz a voz. Também é possível escolher a mesma saída em ambos.

A primeira seleção escolhe a saída padrão de comunicação para voz e, quando possível, outra saída para toque. Mudanças são mantidas em memória e reconciliadas ao desconectar dispositivos, sem gravar identificadores no Windows Registry. Testes: política de papéis e fallback, UI smoke com dois endpoints sintéticos em direções opostas e caixa-preta confirmando presença dos controles mesmo sem hardware.

**Não há implementação de toque ou áudio SIP** no Baresip da aplicação; isso é um contrato de UX e estado local para integração posterior. O Baresip oferece opções documentadas `audio_player` e `audio_alert` com limitações de versão/módulo, cuja compatibilidade terá de ser testada em uma instância de telefonia real. O botao Chamar continua bloqueado.

## v0.49 — Botões TESTAR TOQUE e TESTAR VOZ

Os dois seletores de saída agora permitem reproduzir um tom curto no endpoint escolhido via renderização WASAPI (modo compartilhado do Windows). Não muda a saída padrão do sistema, não grava IDs, não acessa o microfone e não envia comandos ao Baresip. A execução ocorre em worker limitado e só após clique explícito, com erro local caso não haja dispositivo suportado. Teste de geração de amostras mono/estéreo, limites de volume e silêncio fora do sinal somado à suíte. O áudio ouvido comprova somente a seleção do endpoint Windows; chamadas SIP e áudio remoto não estão implementados.

## v0.52 — Volume separado para testes dos dois destinos

Seletores independentes de 25%, 50%, 75% e 100% do **som de teste** de toque/voz. Implementação sobre o tom local existente, multiplicando amostras PCM16 ou float32 pela fração escolhida. A UI não modifica volume mestre do Windows nem controle de áudio do Baresip. Default 75% do tom seguro. Unit test cobrindo níveis válidos/inválidos, ambos formatos e saída mono/estéreo; smoke Windows cobre seleção sem dispositivo físico. Toda mídia de chamada SIP permanece bloqueada até integração nativa.

## v0.53 — Admissão segura de campainha para chamada entrante

O novo gate aceita um toque automático somente quando `SipRuntimeObservation` comprova processo em execução, `VerifiedNativeApi`, sessão válida, estado `Ringing`, token opaco da chamada compatível com `last_call_id` e saída de toque ativa. A transição para chamada atendida, falha, expiração da sessão, remoção da saída e reinício exige Stop. Não há sobreposição de sessões: se a chamada muda, primeiro se interrompe a anterior. Testes de unidade e de integração com mailbox/projeção sintéticos cobrem esses cenários, além de overflow fail-closed.

**Entrega apenas de segurança/estado, não som automático.** O aplicativo ainda não recebe callbacks SIP reais e não possui um player de campainha conectado a essas decisões. Os botões manuais TESTAR TOQUE/TESTAR VOZ seguem funcionando independentemente. O campo `audio_alert` não pode ser tomado isoladamente como mapeamento comprovado de chamadas entrantes; requer comprovação com versão e módulos do Baresip.

## v0.54 — Fence de endpoint do toque por sessão e chamada

O gate `IncomingRingtoneGate` mantém internamente o endpoint Windows autorizado, sem persistência. Se a seleção mudar de PC para headset (ou vice-versa), ou se a saída ficar indisponível, o estado cancela a autorização e exige um ciclo Stop antes de uma futura autorização Start. Inversão de toque/voz e seleção manual forçam reavaliação no loop UI. A observação normal por timer também verifica hotplug. Os testes cobrem troca durante chamada recebida sintética, desconexão do endpoint, reautorização, revogação da evidência e overflow da fila nativa.

A saída ainda não é controlada por eventos reais do Baresip neste executável: somente a intenção segura foi implementada, sem campainha SIP real.

## v0.55 — Contrato de parada confirmada de campainha

O novo coordenador separa a autorização de toque de sua reprodução. O destino não pode ser alterado se o reprodutor da rota antiga não confirmar a parada. Falha ao iniciar também aciona parada obrigatória antes de qualquer outra tentativa; a mesma chamada não dispara retries automáticos. A implementação suporta injeção de reprodutor de teste para testes sem dispositivo físico. No build principal, o reprodutor está **desabilitado**, porque ainda não existe callback SIP nativo auditado integrado. A UI preserva o comportamento de testes locais, sem campainha de telefonia.
