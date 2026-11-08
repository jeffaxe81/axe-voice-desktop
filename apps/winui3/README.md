# AXE Voice — edição WinUI 3

## Estágio 2B — candidato opcional com host Baresip real

Há duas distribuições independentes:

- `AXE-Voice-WinUI3-Preview-No-SIP`: prévia convencional, sempre offline.
- `AXE-Voice-WinUI3-Native-SIP-Lab-Candidate`: contém o mesmo projeto WinUI 3,
  mas com a `axv_frontend_bridge.dll` construída contra Baresip/libre
  auditados. **Apenas laboratório**, nunca produção sem homologação.

A versão nativa **não registra automaticamente**. Clique em **Conectar**
somente depois de salvar, na edição Win32 nativa atualizada, servidor,
usuário, porta, transporte UDP ou TLS, CA (quando TLS) e uma senha no
Windows Credential Manager com o checkbox de lembrar senha. A senha não
passa pela camada C#, pelo clipboard nem pelos relatórios. Dados antigos
sem transporte explícito são recusados — reabra a configuração no Win32
e salve a conta novamente. Nunca ocorre downgrade TLS→UDP.

É recomendável desconectar a edição Win32 antes de registrar o mesmo ramal
pela WinUI 3. **Ligar/Atender/Recusar/Encerrar** dependem da identidade
de sessão e do evento de registro confirmado pelo SDK; o comando enfileirado
não garante estabelecimento de chamada. O botão **Desconectar** encerra
o reactor antes do fechamento.

Este candidato ainda requer **testes reais** de registro, chamadas
entrantes/saintes, áudio RTP bidirecional, headset, TLS/CA e eventual NAT
com uma central de homologação, preferencialmente Dígitro.


Aplicação Windows independente do AXE Voice Win32, com identidade visual
Essencial Inteligente, janela compacta, teclado 12 teclas, apagar, temas e
navegação local. **A distribuição Preview não registra nem realiza chamadas; a variante Native SIP Lab Candidate tem integração experimental.**

## Tecnologias

- Frontend: C#/.NET 10 + WinUI 3/XAML, Windows App SDK **2.6.5 estável**.
- Núcleo de apresentação: `axv_frontend_bridge.dll` (C++17, C ABI v2),
  exportado de `src/frontend_shared` no mesmo repositório.
- Motor de telefonia: **o mesmo código Baresip/libre do Win32**, incluído
  opcionalmente na DLL de laboratório. A edição Preview permanece offline;
  a variante Native Lab habilita conexão explícita e validação de sessões.

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
inalterados. A presença da bridge Preview não habilita comandos SIP; somente o candidato nativo habilita comandos após registro verificado.

A distribuição unpackaged depende de instalação prévia do Windows App SDK
Runtime correspondente. CI valida compilação; testes com headset/PABX Dígitro
e equivalência funcional WinUI 3 ficam para os próximos estágios.

Fontes de referência:
- https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/unpackage-winui-app
- https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/experimental-channel
- https://github.com/microsoft/winappCli/tree/main/samples/winui-unpackaged-app

## Instalador Windows separado (somente candidato de laboratório)

A CI do perfil nativo pode produzir **dois formatos** de distribuição WinUI 3:
`AXE-Voice-WinUI3-Native-SIP-Lab-Candidate` (ZIP de arquivos) e
`AXE-Voice-WinUI3-Native-Lab-Setup` (instalador Inno Setup por usuário).

O instalador da edição WinUI 3 possui **AppId próprio**. Portanto, não
desinstala nem substitui a edição Win32. O atalho de área de trabalho é
opcional e desmarcado por padrão; o instalador não abre o aplicativo nem
faz registro SIP automaticamente.

O teste em Windows verifica arquivos e hashes instalados, diretório de
licenças, instalação/desinstalação e criação/remoção de atalho somente
quando solicitado. Para abrir a edição WinUI 3, o computador ainda
precisa do **.NET 10 Desktop Runtime** e **Windows App Runtime 2.6.5**
compatíveis; este instalador candidato não distribui nem instala os
runtimes e não representa a edição comercial.

Nunca inserir senhas na linha de comando de instalação. O aplicativo
usa, mediante consentimento explícito, credenciais salvas pelo AXE Voice
Win32 no Gerenciador de Credenciais do usuário Windows atual.
