# AXE Voice Desktop — Windows, áudio primeiro (v0.43.0 laboratório)

Protótipo de softphone SIP corporativo em Windows com motor planejado **Baresip**. A versão atual oferece interface Win32, validação local de parâmetros SIP, verificação offline da presença de baresip.exe e inventário de dispositivos de áudio do Windows.

**Não implementa ainda registro SIP, chamadas, RTP/SRTP, áudio bidirecional ou armazenamento protegido de credenciais.** O botão "Chamar" fica bloqueado por padrão.

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

## v0.46.0 — Atualizacao de headset por notificacao Windows

O painel ÁUDIO reage a eventos `WM_DEVICECHANGE`, agrupa notificações com debounce de 400 ms e inspeciona dispositivos ativos somente quando a aba está visível. Reabrir a aba força atualização imediata. A seleção continua efêmera e não é aplicada ao motor Baresip. O botão manual continua disponível porque `WM_DEVICECHANGE` não é completo para alterações de endpoint padrão. Não há chamada SIP ou áudio RTP/SRTP funcional.

## v0.47.0 — Notificações Core Audio de dispositivos padrão

O painel ÁUDIO agora registra um `IMMNotificationClient` oficial do Windows para detectar mudanças no dispositivo padrão de comunicação, além de adição, remoção, estado e propriedades de endpoints. O callback apenas envia uma mensagem interna à interface; o timer existente agrupa alterações e atualiza o inventário do Windows, sem acessar o Core Audio no callback. A subscrição é encerrada ao destruir a janela; falhas no registro não impedem atualização manual ou via `WM_DEVICECHANGE`. Não utiliza dados dos endpoints fora da memória, não configura áudio do Baresip nem habilita chamadas.
