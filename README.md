# AXE Voice Desktop — Windows LAB (v0.154.0, desenvolvimento)

Protótipo de softphone corporativo Win32 com áudio local, preferências separadas para voz/toque, inventário de dispositivos Windows, atalhos de discador e pré-verificações SIP.

**Estado real:** o aplicativo `AXEVoice.exe` mantém o motor `UnavailableSipEngine` e o botão Chamar bloqueado, pois não há prova de registro SIP autenticado, TLS de servidor validado, SRTP ou áudio de chamada. As credenciais SIP podem ser administradas no cofre Windows local; a integração do SDK Baresip para autenticação existe somente no alvo experimental, desabilitado por padrão e ainda não homologado.

**O código v0.154 não foi compilado nem testado nesta rodada.** A arquitetura Baresip é trabalho de laboratório; não confundir o percentual de entregas de código com aceite operacional. A `main` mantém a base histórica MicroSIP/PJSIP; promoção exige os [critérios documentados](docs/AXEVOICE-SIP-SDK-HARDENING-V116-V120-UNTESTED.md).

A evolução v0.126–v0.130 reforça o controle experimental das notificações nativas: exige a ordem REGISTERING/REGISTER_OK, conserva erros de registro por sessão, reconhece rejeição imediata e revoga a sessão se a confirmação de registro desaparecer. **Nenhuma dessas melhorias habilita chamadas no aplicativo.**

## Reconciliação de rascunhos — v0.131–v0.134

O código incorpora seletivamente proteções existentes nas PRs históricas #39, #18 e #16 à branch atual, **sem mesclar suas implementações antigas por inteiro**: limite de lote SIP antes da iteração; preservação de IDs locais ao falhar um scan Core Audio; preferência explícita de headset que volta a ser selecionada quando reconectado; atualização de diagnóstico/versão.

O motor SIP e as chamadas continuam indisponíveis. A compilação Windows, os testes e a homologação permanecem pendentes.

## Reconciliação adicional — v0.135–v0.139

O adaptador Baresip experimental agora valida com `call_get_ua` a conta proprietária da chamada tanto ao iniciá-la quanto em eventos de estabelecimento/encerramento, recuperando a proteção da PR histórica #28. A API possui uma verificação de assinatura no probe de SDK, e o header dos simuladores foi ajustado apenas para compatibilidade com essa chamada.

O relatório de prontidão F2 passa a usar o formatador de indicadores booleanos da PR histórica #21, sem dados de identidade, senha ou destino. O diagnóstico local para área de transferência ganhou limite de 8.192 caracteres. Nenhuma das PRs antigas foi mesclada integralmente ou fechada.

**Motor SIP nativo, autenticação, RTP e áudio real seguem indisponíveis no executável; nenhuma compilação ou teste nesta rodada.**

## Rascunhos #30, #31 e #40 — integração seletiva v0.140–v0.144

- Reconhece adicionalmente `DBT_CONFIGCHANGED` sem tratar a notificação como prova de mídia SIP.
- Observa mudanças de dispositivos padrão do Windows Core Audio (`IMMNotificationClient`) para comunicação e multimídia; mensagens da COM apenas notificam a interface, sem dados de conta ou identificação de dispositivos.
- Reutiliza o debounce, limita a atualização à aba de Áudio visível e interrompe os testes locais de saída antes da mudança de rota.
- Faz `UnregisterEndpointNotificationCallback` ao destruir a janela; falha na inicialização mantém atualização manual e aviso genérico `WM_DEVICECHANGE`.
- O filtro de callbacks Baresip verifica a geração e a sessão atual do mailbox antes de admitir um callback nativo.

**Sem compilar nem executar testes nesta rodada**. Não implica homologação de headset, áudio real SIP ou autenticação com PABX. Os rascunhos históricos permanecem abertos porque o merge é parcial.

## PR #30 — consultas Core Audio assíncronas v0.145–v0.149

A aba Áudio agora **agenda** a enumeração dos dispositivos Windows em uma thread auxiliar, em vez de chamar a API Core Audio de forma síncrona no thread da interface. As consultas têm geração/epoch; resultados antigos são descartados quando a aba é deixada ou outra atualização é solicitada. Pedidos manuais feitos enquanto há consulta pendente são agrupados e retomados após o término da consulta anterior. O resultado é aplicado na interface somente quando a aba Áudio estiver visível e a prévia de áudio estiver parada.

A seleção explícita de headset/saída de voz/toque continua preservada contra falhas temporárias. Não há mudança de motor SIP, autenticação, RTP, SRTP ou chamada real. **Código não compilado nem testado nesta rodada**; o restante da PR antiga #30 continua em rascunho.

## Reconciliação PR #30 — diagnóstico de áudio em Ajustes v0.150–v0.154

O botão **Dispositivos de áudio** na aba Ajustes deixa de bloquear a interface com a enumeração Windows Core Audio. A verificação usa uma thread auxiliar separada da aba Áudio; cada aba mantém a própria geração de consulta e sua própria solicitação pendente. Resultados obsoletos são descartados ao sair de Ajustes ou quando houver novo clique, e novas verificações são agrupadas em vez de iniciar varreduras concorrentes. A resposta é publicada na interface apenas por um timer Win32 na aba Ajustes visível.

O relatório anônimo para copiar checklist já havia sido reconciliado na PR #119; não foi duplicado. O botão Chamar continua bloqueado com `UnavailableSipEngine`. **Não houve compilação, testes ou homologação nesta rodada.**

## Compilar e abrir no Windows

Requer CMake no PATH, Visual Studio 2022 Build Tools, workload Desktop development with C++, MSVC e Windows SDK.

No PowerShell, a partir da raiz do repositório:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1 -Run

Ou manualmente:

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --parallel 4
    ctest --test-dir build -C Release --output-on-failure
    & ".\build\Release\AXE-Voice.exe"

Executável: build\Release\AXE-Voice.exe. O CI publica um artefato do protótipo, não um instalador de produção.

## Validação v0.43 — unitários, caixa-preta e integração

A CI Windows executa a suíte CTest completa e depois três grupos independentes: **unit**, **blackbox** e **integration** via `scripts/test-quality-gates.ps1`. Um teste caixa-preta cria um **processo externo real** `AXE-Voice.exe --ui-blackbox` e usa as mensagens Win32 para verificar 100 entradas, feedback e bloqueios, além de navegar nas telas e testar o encerramento. A integração usa as políticas reais de SIP, fila de eventos, projeção e motor indisponível, com 100 tentativas adicionais e revogação de sessões. Nenhum desses testes conecta a um PABX.

**100% de aprovação** significa que todos os testes executados passaram — não é 100% de cobertura de código nem homologação SIP/RTP ou áudio bidirecional real.

## Frontend DPI v0.42
Interface, fontes, botões e modo MINI se adaptam a monitores com DPI diferentes. A rotina de atualização usa coordenadas lógicas fixas de 96 DPI e acompanha WM_DPICHANGED, com teste Win32 de reflow sintético em 150% e 200%. Não há liberação de chamadas SIP. Falta validação manual em monitores físicos.

## Frontend acessível v0.40
A interface adapta suas cores automaticamente ao modo Alto Contraste do Windows, incluindo mudanças durante a execução. Os estados SIP continuam distintos do estado do processo e nenhum controle de chamada é desbloqueado por alterações de aparência. Ver [documentação de frontend](docs/FRONTEND-WINDOWS.md). Ainda falta homologação manual com leitores de tela e escalas de DPI.

## Interface de softphone v0.27

A v0.27 prepara uma ponte para eventos nativos do Baresip: registro, falha, chamadas recebidas e realizadas, conexão e encerramento. O modelo inclui estado **Ringing** para ligações entrantes, valida a ordem dos eventos e impede eventos de sessões antigas. A caixa thread-safe de 32 eventos limita o consumo de memória e bloqueia a sessão se houver excesso de eventos. **Não existe produtor Baresip nativo nesta versão:** os eventos nos testes são sintéticos, o motor permanece indisponível e Chamar/Atender continuam desabilitados.

## Interface de softphone v0.26

A v0.26 exibe o estado real do processo local Baresip na faixa superior: parado, ativo, encerrando ou falha. O início do processo **não é evidência de registro SIP**. O botão Chamar permanece bloqueado até futura confirmação por adaptador nativo validado e sessão atual. Texto de console/log não autentica o registro.

## Interface de softphone v0.24

O frontend Win32 foi reorganizado em duas telas, **Teclado** e **Ajustes**, com teclado telefônico completo (0–9, * e #), apagar/limpar destino, tema claro/escuro e opção de janela sempre no topo (FIXAR/SOLTAR), comandos visuais de mudo/espera/transferência/encerrar (intencionalmente desativados) e preferências de tema/fixação salvas em HKCU. Os comandos são acessíveis pelo teclado e a entrada aceita ramais ou SIP URI.

Na v0.25, a aba **ÁUDIO** consulta nomes de microfones e saídas do Windows Core Audio. Permite escolher uma preferência **somente em memória** com retorno seguro ao dispositivo padrão após desconexão. **Não configura nem encaminha áudio no Baresip**.

O modo **MINI/MAX** da v0.24 oferece uma janela reduzida com destino e status SIP. Abrir **Ajustes** expande automaticamente a janela. Apenas a preferência compacta é persistida em HKCU, sem salvar número, histórico ou credenciais.

Atalhos: **Ctrl+K** (teclado), **Ctrl+L** (foco no número), **Ctrl+,** (ajustes), **Ctrl+T** (tema), **Ctrl+P** (fixar), **Ctrl+M** (mini/max); Tab/Shift+Tab percorrem controles. A opção **Chamar** permanece propositalmente desativada: nenhuma ligação real ocorre com o motor atual. A janela pode ser minimizada para a bandeja do Windows (duplo clique no ícone para restaurar; menu com Abrir/Sair); se a bandeja não estiver disponível, a minimização normal permanece. Os diagnósticos de Baresip e de áudio seguem disponíveis em Ajustes; os indicadores distinguem processo local de registro SIP. Ver [guia do frontend](docs/FRONTEND-WINDOWS.md).

## Diagnósticos

- **Parada e fechamento (v0.20)**: `/quit` é enfileirado em uma thread Win32 própria, sem escrita no pipe pela interface. Ao fechar a janela, a UI acompanha a saída a cada 100 ms, com prazo de 1,5 s e fallback por Job Object. A confirmação de execução do comando pelo Baresip ainda está pendente.
- **Iniciar Baresip (laboratório)**: executa o processo com perfil separado em LocalAppData/AXEVoice/baresip-lab; bloqueia contas SIP pré-configuradas e módulos de controle remoto não autenticados. Ainda não estabelece chamadas.
- **Verificar Baresip local**: audita a presença de módulos locais G.711, G.722, Opus, WASAPI, WinWave e SRTP, sem carregar ou autenticar bibliotecas. Não certifica codecs disponíveis em chamada.
- **Verificar microfone e saída de áudio**: conta dispositivos ativos e identifica os padrões de comunicação no Windows. Não grava, reproduz nem confirma a ligação com Baresip.

Documentação: [Auditoria de módulos e códigos de saída](docs/BARESIP-MODULE-AUDIT.md), [Perfil de laboratório](docs/BARESIP-LAB-PROFILE.md), [Áudio](docs/AUDIO-DEVICES.md), [Integração Baresip](docs/BARESIP-INTEGRATION.md), [Checklist de release](docs/RELEASE-CHECKLIST.md).

G.711, G.722 e Opus são alvos do MVP, ainda não homologados; G.729 exige avaliação técnica e de licenciamento. Vídeo e chat são posteriores.

## Histórico/licenciamento

O histórico do repositório inclui pesquisa da baseline MicroSIP/PJSIP sob GPL, que não será usada como backend proprietário sem auditoria de licenças. Antes da distribuição comercial, avaliar dependências, licenças e isolamento do código. Nunca incluir segredos ou credenciais SIP reais no Git.
