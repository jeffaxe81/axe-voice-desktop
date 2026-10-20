# AXE Voice Desktop — Windows, áudio primeiro

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

## Diagnósticos

- **Verificar Baresip local**: verifica apenas a existência de arquivo regular baresip.exe ao lado do AXE-Voice.exe. Não executa nem autentica o arquivo.
- **Verificar microfone e saída de áudio**: conta dispositivos ativos e identifica os padrões de comunicação no Windows. Não grava, reproduz nem confirma a ligação com Baresip.

Documentação: [Áudio](docs/AUDIO-DEVICES.md), [Integração Baresip](docs/BARESIP-INTEGRATION.md), [Checklist de release](docs/RELEASE-CHECKLIST.md).

G.711, G.722 e Opus são alvos do MVP, ainda não homologados; G.729 exige avaliação técnica e de licenciamento. Vídeo e chat são posteriores.

## Histórico/licenciamento

O histórico do repositório inclui pesquisa da baseline MicroSIP/PJSIP sob GPL, que não será usada como backend proprietário sem auditoria de licenças. Antes da distribuição comercial, avaliar dependências, licenças e isolamento do código. Nunca incluir segredos ou credenciais SIP reais no Git.
