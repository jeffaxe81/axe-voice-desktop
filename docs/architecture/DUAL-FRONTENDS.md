# AXE Voice — duas interfaces Windows

## O que existe nesta fase

| Edição | Frontend | Backend | Distribuição |
| --- | --- | --- | --- |
| **AXE Voice Win32 Essencial** | C++17 + Win32 | Baresip/libre nativo existente no build homologável | Instalador `AXE-Voice-Native-Candidate-Setup` |
| **AXE Voice WinUI 3 Preview** | C#/.NET 10 + XAML, Windows App SDK estável 2.6.5 | **não acoplado** (somente projeção C++/C ABI) | `AXE-Voice-WinUI3-Preview-No-SIP` |

A edição WinUI 3 é **um executável Windows independente**, não uma imagem
simulada, mas nesta etapa sua discagem real permanece desativada. O
`axv_frontend_bridge.dll` expõe somente estados de apresentação e uma versão
de ABI. A edição Win32 continua a usar `essential_call_actions.h`,
reaproveitado pela bridge — nenhum segundo stack SIP foi escrito.

```text
     WIN32 / C++                    WINUI 3 / C# + XAML
 AXEVoice.exe (mantido)          AXEVoice.WinUI3.exe
        |                              |
 essential_call_actions.h     axv_frontend_bridge.dll (C ABI v1)
        |                              |
        +-------- projeção C++17 ------+
        |
  Baresip/libre nativo (somente Win32, por enquanto)
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
O WinUI 3 ainda possui apenas a bridge de **apresentação**, não um host
de telefonia; portanto continua mostrando SIP indisponível. O backend
não foi duplicado nem transferido integralmente para uma DLL.

## Próxima extração do backend

Implementar `VoiceApplicationService` C++ como dono único do Baresip,
com observação imutável e fila validada de comandos. A bridge WinUI 3
deverá receber sessões/status originados exclusivamente desse serviço.

Segurança: não expor `UA*`, callbacks nativos ou buffers de senha no ABI;
C# nunca armazena segredo ou gera indicação positiva a partir de estado
inventado. Credenciais devem ficar sob proteção Windows Credential Manager
e do backend C++; comandos exigem token/estado verificados na entrada.

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
