# AXE Voice — edição WinUI 3 (prévia do estágio 1)

Aplicação Windows independente do AXE Voice Win32, com identidade visual
Essencial Inteligente, janela compacta, teclado 12 teclas, apagar, temas e
navegação local. **Não faz chamadas nem registra SIP nesta fase.**

## Tecnologias

- Frontend: C#/.NET 10 + WinUI 3/XAML, Windows App SDK **2.6.5 estável**.
- Núcleo de apresentação: `axv_frontend_bridge.dll` (C++17, C ABI v1),
  exportado de `src/frontend_shared` no mesmo repositório.
- Motor de telefonia: **Baresip/libre existente** no projeto Win32.
  Ainda **não está conectado** a este frontend; nenhum segundo SIP stack.

## Compilar (Windows 10/11 x64)

Pré-requisitos: .NET 10 SDK, Windows SDK 26100, WinUI Windows App SDK runtime
versão compatível e Visual Studio/Build Tools com componentes Windows.

```powershell
cmake -S . -B build-win32 -A x64 -G "Visual Studio 17 2022"
cmake --build build-win32 --config Release --target axv_frontend_bridge
dotnet restore apps/winui3/AXEVoice.WinUI3.csproj -p:Platform=x64
dotnet build apps/winui3/AXEVoice.WinUI3.csproj -c Release -p:Platform=x64
# Copiar a DLL ao diretório de saída da aplicação, quando necessário.
```

**Distinção de releases:** o executável desta edição será entregue como
`AXE-Voice-WinUI3-Preview`. O executável Win32 nativo e o instalador
`AXE-Voice-Native-Candidate-Setup` são independentes e permanecem
inalterados. A presença da bridge não habilita comandos SIP.

A distribuição unpackaged depende de instalação prévia do Windows App SDK
Runtime correspondente. CI valida compilação; testes com headset/PABX Dígitro
e equivalência funcional WinUI 3 ficam para os próximos estágios.

Fontes de referência:
- https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/unpackage-winui-app
- https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/experimental-channel
- https://github.com/microsoft/winappCli/tree/main/samples/winui-unpackaged-app
