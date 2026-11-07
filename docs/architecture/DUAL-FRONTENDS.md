# AXE Voice — duas interfaces Windows

## O que existe nesta fase

| Edição | Frontend | Backend | Distribuição |
| --- | --- | --- | --- |
| **AXE Voice Win32 Essencial** | C++17 + Win32 | Baresip/libre nativo existente no build homologável | Instalador `AXE-Voice-Native-Candidate-Setup` |
| **AXE Voice WinUI 3 Preview** | C#/.NET 10 + XAML | Offline; C ABI apenas para apresentação | `AXE-Voice-WinUI3-Preview-No-SIP` |
| **AXE Voice WinUI 3 Native Lab** | Mesmo frontend WinUI 3 | Código Baresip/libre nativo, conexão manual | `AXE-Voice-WinUI3-Native-SIP-Lab-Candidate` |

A edição WinUI 3 é um executável Windows independente, não uma imagem
simulada. A **Preview** permanece offline, enquanto a **Native Lab** usa a
DLL SDK real para registrar sob ação explícita e enviar comandos protegidos
pela identidade da sessão. Esta variante requer homologação antes de produção. A edição Win32 continua a usar `essential_call_actions.h`,
reaproveitado pela bridge — nenhum segundo stack SIP foi escrito.

```text
     WIN32 / C++                    WINUI 3 / C# + XAML
 AXEVoice.exe (mantido)          AXEVoice.WinUI3.exe
        |                              |
 essential_call_actions.h     axv_frontend_bridge.dll (C ABI v2)
        |                              |
        +-------- projeção C++17 ------+
        |
  Baresip/libre nativo (Win32 + variante WinUI3 Native Lab)
```

## Estágio 2A — camada de aplicação C++ compartilhada

O arquivo `src/voice_application_service.h` agora concentra a projeção
imutável de status e os comandos **Ligar, Atender e Encerrar**. A edição Win32
consome essa fachada e continua utilizando **a mesma instância Baresip**,
sem alterações de senha, codecs, conta ou motor. Toda ação é verificada
contra a sessão e a identidade da chamada exibidas antes de chegar ao
SDK; o próprio motor verifica novamente na fila nativa.

Os testes `tests/test_voice_application_service.cpp` cobrem 2.000 comandos
com sessão válida e 2.000 obsoletos, além de estados inválidos, rejeição
de destino e falha da fila. São testes simulados, sem chamadas externas.

**Limite importante:** o `VoiceApplicationService` deste estágio é uma
fachada **não proprietária** sobre o Baresip, já consumida pelo Win32.
Na etapa 2A o WinUI 3 possuía apenas a bridge de apresentação. No
estágio 2B, a variante Native Lab adicionou seu host nativo, enquanto
a edição Preview permanece offline. O backend
não foi duplicado nem transferido integralmente para uma DLL.

## Próximos passos

Homologar registro, chamadas, áudio bidirecional e TLS com central Dígitro
e headset físico; depois integrar Mudo/Espera/Transferência, monitor de RTP
e configuração de contas segura diretamente no WinUI 3.

Segurança: não expor `UA*`, callbacks nativos ou buffers de senha no ABI;
C# nunca armazena segredo ou gera indicação positiva a partir de estado
inventado. Credenciais devem ficar sob proteção Windows Credential Manager
e do backend C++; comandos exigem token/estado verificados na entrada.

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

## Como testar

1. Win32 — continuar usando os workflows `AXE Voice Windows CI` e
   `AXE Voice native SDK smoke`, mais homologação externa.
2. WinUI 3 — workflow `AXE Voice WinUI3 Preview`: CMake compila DLL,
   CTest valida matriz de 2.000 estados válidos e 2.000 obsoletos,
   .NET publica aplicativo unpackaged x64.
3. Instalar Windows App Runtime estável correspondente na máquina, se
   não estiver presente; abrir `AXEVoice.WinUI3.exe` e verificar que
   **não há botão Ligar habilitado** sem motor SIP conectado.
4. Revisão visual lado a lado: janela compacta, fontes, teclado e tema.
5. Homologação futura com central Dígitro física para as duas edições.

Referências:
- https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/unpackage-winui-app
- https://learn.microsoft.com/en-us/windows/apps/windows-app-sdk/experimental-channel
