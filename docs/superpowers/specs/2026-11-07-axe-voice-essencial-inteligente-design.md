# AXE Voice Desktop — Design: Essencial inteligente

**Status:** proposta de design aguardando aprovação; **não é implementação nem homologação**.
**Data:** 2026-11-07.
**Repositório:** `jeffaxe81/axe-voice-desktop` (C++17, Win32, Baresip nativo).
**Origem:** seleção explícita da abordagem B (Essencial inteligente), preservando a opção 1 anteriormente aprovada.

## 1. Objetivo e critérios

Melhorar clareza, ergonomia e acessibilidade da interface Win32, sobretudo para **chamadas entrantes** e **chamadas em andamento**, sem descaracterizar o discador compacto.

- Manter o corpo da janela Essencial em **355 × 602 pixels lógicos a 96 DPI** (janela externa) e o escalonamento existente para 125%/150%. Sem rolagem vertical/horizontal.
- Preservar o visual aprovado: fundo claro como padrão, tema escuro opcional, cantos arredondados, teclado próximo, cores e marca AXE Voice, mesma navegação de Monitor SIP e Preferências.
- Nada pode sobrepor textos, campos ou botões; um único conjunto de comandos e motor SIP; **não criar um segundo softphone**.
- A maior ação visível sempre deve corresponder à ação mais útil no **estado SIP verificado**.
- Preservar funcionalidade de conta SIP, codecs, gravação opcional de credencial com Windows Credential Manager, áudio, monitor e discagem.

## 2. Abordagens consideradas

| Alternativa | Vantagens | Limitações |
|---|---|---|
| A — Refinamento visual | Baixo risco; fidelidade máxima | Não resolve comandos de atender/encerrar ocultos |
| **B — Essencial inteligente (selecionada)** | Mantém aparência e dimensões, expondo ação contextual | Requer matriz de estados, ajustes de disposição e testes UI |
| C — Modernização ampla | Mais liberdade de design | Maior risco de descaracterizar opção 1 e comprometer layout compacto |

## 3. Observações verificadas no código atual

- `kEssentialScalePercent=80`, `kEssentialWidth=355` e `kEssentialHeight=602`.
- `apply_faleweb_layout` posiciona discador, teclas, controles e navegação, porém **oculta** `kHangupFuture` e, na versão nativa, `kAnswerNative` na tela Essencial.
- `refresh_runtime_banner` recebe a observação SIP da sessão e é executado nas atualizações da UI; existem comandos nativos de atender, encerrar, mudo e espera com validações de sessão/estado.
- As preferências possuem seis abas e o Monitor SIP acompanha **estados locais observados**, não mensagens SIP brutas.

## 4. Composição visual aprovada — preservada

1. **Cabeçalho:** logotipo/nome AXE Voice, tema e acesso ao Monitor, sem novos controles permanentes.
2. **Faixa de estado SIP:** texto e indicador visual semanticamente distintos: desconectado/erro, registrando, registrado, chamando, tocando, em chamada. **Verde só quando o SDK confirmar o registro**.
3. **Campo destino + teclado:** 12 teclas nos mesmos três alinhamentos e proporções compactas, mantendo a área de digitação original.
4. **Faixa primária:** a área atual do botão Ligar troca de função ou se subdivide apenas quando houver chamada.
5. **Faixa secundária:** controles existentes Mudo/Espera/Transferir somente quando há chamada estabelecida e comando autorizado.
6. **Rodapé/navegação:** mesma linha de Teclado, Contatos, Histórico e Ajustes; Monitor segue acessível sem alterar dimensões da janela.

A interface usa Segoe UI, textos curtos com elipse somente como último recurso, ícones opcionais apenas se adicionarem clareza e cores sem codificar o estado exclusivamente por cor. Não acrescentar barra de rolagem, banners extras nem rodapés de logs.

## 5. Matriz dos estados do discador

| Evidência SIP | Ações primárias | Secundárias e comportamento |
|---|---|---|
| Motor parado, erro ou registro não comprovado | **Ligar desabilitado** | Indicador não-verde; nenhuma ação de chamada artificialmente habilitada |
| Registrando | **Ligar desabilitado** | Texto "Registrando…", sem afirmar conexão |
| Registrado e sem chamada | **Ligar** se destino e autorização válidos | Teclado e campo editáveis; ações exclusivas de chamada desabilitadas/ocultas |
| Chamada recebida, comprovada (`Ringing`) | **Atender** (verde) e **Recusar** (vermelho) lado a lado na faixa primária | Não invocar dial ao pressionar; campo/teclado não devem gerar chamada paralela |
| Chamada originada (`Calling`) | **Encerrar** (vermelho), no lugar de Ligar | "Chamando…" sem mostrar cronômetro como se já houvesse atendimento |
| Chamada estabelecida (`InCall`) | **Encerrar** (vermelho) | **Mudo**, **Espera** e **Transferir** na faixa secundária, com feedback da operação; duração somente após evento nativo |
| Chamada encerrada, sessão trocada ou transporte perdido | Retorna ao estado efetivamente comprovado | Zerar affordances de chamada, evitando botões ativos após eventos atrasados |

**Regra de segurança:** a projeção da UI utiliza `current_sip_observation` e os mecanismos existentes de prova/identidade de sessão (`SipEvidence`, `call_id`, `session`), jamais contadores ou estados inferidos de logs. O clique reaplica as autorizações nativas existentes, inclusive após mudanças entre render e ação. **Não usar visibilidade de botão como mecanismo de autorização.**

## 6. Comportamento e microinterações

- Pressionar uma tecla produz breve resposta visual, sem animações que ocultem números ou se sobreponham ao texto.
- Atualizações por temporizador só reposicionam controles quando **o estado visual efetivamente mudar**, para evitar flicker, foco perdido e redraw repetitivo.
- Navegação pelo teclado e foco visível; ordem Tab compreensível; Enter/Espaço respeitam o controle selecionado.
- Acionamentos de Atender/Recusar/Encerrar só reutilizam **IDs e handlers nativos já existentes**. Para resposta a chamadas, preservar a referência à chamada visível e impedir aceitar/encerrar outra sessão por acidente.
- Se uma chamada chegar enquanto Monitor ou Preferências está aberto, emitir aviso visual não intrusivo e manter um caminho seguro para responder. Não preencher a tela com dados pessoais do chamador.
- Para erros do SIP, informar condição real ("Sem registro", "Falha de transporte", etc.), sem alegar mídia ou RTP bem-sucedido com base apenas no registro.

## 7. Layout, ergonomia e contraste

- Manter espaçamento uniforme entre teclas, bordas arredondadas consistentes, áreas de clique suficientemente grandes e no mínimo 3–5 pixels lógicos de separação entre botões contíguos após escala.
- Verificar as dimensões finais de **área cliente** após borda/título do Windows; nenhum botão pode ficar fora do viewport a 100%, 125%, 150% de DPI nem com tema escuro ou alto contraste.
- Checar títulos em português ("Atender", "Recusar", "Encerrar", "Transferir") com fontes reais e `DrawTextW` / `GetTextExtentPoint`; evitar truncamento e sobreposição.
- Garantir que cinza/desabilitado, foco, ativo, pressionado e destacado mantenham distinção mesmo em alto contraste.
- Não migrar o frontend para HTML/Electron ou substituir o layout aprovado por uma interface genérica.

## 8. Implementação prevista (somente após aprovação)

1. Criar projeção pura e testável da apresentação do discador a partir de `SipRuntimeObservation` e autorização de comandos, sem IO.
2. Fazer a `apply_faleweb_layout` posicionar os controles conforme a projeção; reaproveitar os mesmos IDs, comandos e callbacks.
3. Consolidar reaplicação de layout apenas para transições visuais, preservando o foco sempre que seguro.
4. Validar proporções, legendas, temas, alto contraste, DPI e monitor, evitando regressões no modo `--faleweb-ui-smoke`.
5. Publicar instalador **candidato** somente após CI e smoke do SDK; homologação real PABX/RTP é separada.

## 9. Testes e critérios de aceite

- Teste unitário de **todas as linhas da matriz de estado** e rejeição de observação com sessão antiga.
- Testes UI Win32/caixa-preta das coordenadas sem sobreposição para 96/120/144 DPI, claro/escuro e alto contraste; manter janela sem scroll.
- **Recepção:** renderizar Atender/Recusar somente com chamada confiável; validar clique no handler correto; simular chamada terminando entre renderização e clique.
- **Originação:** Ligar desabilitado sem registro; apresentar Encerrar quando Calling; em InCall mostrar controles autorizados; falha volta a estado seguro.
- **Monitor/Preferências:** navegação mantém os controles SIP e não descarta uma chamada entrante.
- Reutilizar 1.000 cenários inbound + 1.000 outbound já existentes (sintéticos) e acrescentar testes de projeção de UI; **não alegar homologação de chamadas reais**.
- Passar baseline, Windows CI e smoke nativo; revisão de código e merge à `main` apenas após tudo verde.
- Comparar capturas do candidato contra opção 1 aprovada antes de declarar fidelidade visual.

## 10. Fora de escopo, riscos e dependências

**Fora de escopo:** novos codecs, gravação, URA, multi-contas, nova aparência geral, autoatendimento, chamada automática para ambientes externos, mudanças na autenticação, SIP/RTP customizado e novas integrações.

**Riscos principais:** controles ocultos em chamada entrante, atualização do layout durante operação, janela menor que a área cliente disponível e falso estado "conectado". Mitigar com função de projeção pura, gating de sessão, testes de coordenadas e aceitação visual.

**Dependência de decisão:** aprovação desta especificação pelo usuário. **Nenhum código de produção é alterado nesta fase.**
