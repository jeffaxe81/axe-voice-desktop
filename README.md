# AXE Voice Desktop — Windows, áudio primeiro (v0.23.0 laboratório)

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

## Interface de softphone v0.23

O frontend Win32 foi reorganizado em duas telas, **Teclado** e **Ajustes**, com teclado telefônico completo (0–9, * e #), apagar/limpar destino, tema claro/escuro e opção de janela sempre no topo (FIXAR/SOLTAR), comandos visuais de mudo/espera/transferência/encerrar (intencionalmente desativados) e preferências de tema/fixação salvas em HKCU. Os comandos são acessíveis pelo teclado e a entrada aceita ramais ou SIP URI.

Atalhos: **Ctrl+K** (teclado), **Ctrl+L** (foco no número), **Ctrl+,** (ajustes), **Ctrl+T** (tema), **Ctrl+P** (fixar); Tab/Shift+Tab percorrem controles. A opção **Chamar** permanece propositalmente desativada: nenhuma ligação real ocorre com o motor atual. A janela pode ser minimizada para a bandeja do Windows (duplo clique no ícone para restaurar; menu com Abrir/Sair); se a bandeja não estiver disponível, a minimização normal permanece. Os diagnósticos de Baresip e de áudio seguem disponíveis em Ajustes; os indicadores distinguem processo local de registro SIP. Ver [guia do frontend](docs/FRONTEND-WINDOWS.md).

## Diagnósticos

- **Parada e fechamento (v0.20)**: `/quit` é enfileirado em uma thread Win32 própria, sem escrita no pipe pela interface. Ao fechar a janela, a UI acompanha a saída a cada 100 ms, com prazo de 1,5 s e fallback por Job Object. A confirmação de execução do comando pelo Baresip ainda está pendente.
- **Iniciar Baresip (laboratório)**: executa o processo com perfil separado em LocalAppData/AXEVoice/baresip-lab; bloqueia contas SIP pré-configuradas e módulos de controle remoto não autenticados. Ainda não estabelece chamadas.
- **Verificar Baresip local**: audita a presença de módulos locais G.711, G.722, Opus, WASAPI, WinWave e SRTP, sem carregar ou autenticar bibliotecas. Não certifica codecs disponíveis em chamada.
- **Verificar microfone e saída de áudio**: conta dispositivos ativos e identifica os padrões de comunicação no Windows. Não grava, reproduz nem confirma a ligação com Baresip.

Documentação: [Auditoria de módulos e códigos de saída](docs/BARESIP-MODULE-AUDIT.md), [Perfil de laboratório](docs/BARESIP-LAB-PROFILE.md), [Áudio](docs/AUDIO-DEVICES.md), [Integração Baresip](docs/BARESIP-INTEGRATION.md), [Checklist de release](docs/RELEASE-CHECKLIST.md).

G.711, G.722 e Opus são alvos do MVP, ainda não homologados; G.729 exige avaliação técnica e de licenciamento. Vídeo e chat são posteriores.

## Histórico/licenciamento

O histórico do repositório inclui pesquisa da baseline MicroSIP/PJSIP sob GPL, que não será usada como backend proprietário sem auditoria de licenças. Antes da distribuição comercial, avaliar dependências, licenças e isolamento do código. Nunca incluir segredos ou credenciais SIP reais no Git.
