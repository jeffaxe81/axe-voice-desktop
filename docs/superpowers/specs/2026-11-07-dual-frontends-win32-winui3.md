# AXE Voice — duas edições Windows sobre um núcleo SIP (design)

**Data:** 2026-11-07 · **Escopo:** fase inicial do desacoplamento · **Status:** solicitado pelo titular.

## Objetivo

Preservar a edição Win32 (Essencial Inteligente, candidato para homologação)
e criar uma **segunda edição real WinUI 3**, com interface independente, para
comparação visual e funcional progressiva. Ambas devem compartilhar o mesmo
código de domínio/telefonia C++, jamais manter forks do motor Baresip.

## Decisões do estágio

- **Win32:** manter o target `AXEVoice`, o executável/instalador atual,
  as preferências e todos os mecanismos SIP nativos, sem mudança de identidade.
- **WinUI 3:** nova aplicação **C#/.NET 10 + XAML**, em `apps/winui3`,
  inicialmente **prévia de UI** com janela compacta, discador e navegação;
  sem registro, origem ou recepção de chamadas reais até a integração nativa.
- **Camada comum:** C++17 (`src/essential_call_actions.h` já usado pelo Win32);
  expor um contrato C ABI versionado e testado em `src/frontend_shared` para
  o WinUI 3, com estados **fail-closed** quando não houver backend.
- **Windows App SDK:** ramo estável, versão de referência 2.6.5 (nov/2026);
  não usar APIs preview/experimental.
- **Distribuição:** dois nomes/artefatos separados no GitHub Actions;
  não substituir silenciosamente o instalador Win32.
- **Segurança:** nada de credenciais no contrato de apresentação; comandos SIP
  continuam com o backend nativo e seus guards por sessão/chamada. Nunca
  simular registro verde ou habilitar Ligar sem estado nativo comprovado.
- **Sem serviços externos:** a edição WinUI 3 não terá um servidor web paralelo.
  O futuro canal WinUI→C++ será in-process com ABI nativa.

## Arquitetura pretendida

```text
AXE Voice Win32 (C++/Win32) ─┐
                             ├─ Domain/application C++ (estados, ações, validação)
AXE Voice WinUI3 (C#/XAML) ─ Bridge C ABI versionada ──┘
                                   │
                            Baresip/libre + WASAPI
```

**Estágio 1:** edição WinUI 3 ligada ao *contrato de apresentação*, inicialmente
com backend indisponível por padrão. Não alegar compartilhamento operacional
do SIP até acoplar comandos e observação reais na bridge.

**Estágio 2:** encapsular o host de SIP/áudio e os comandos por sessão em
`VoiceApplicationService` C++, sem duplicar engine; adicionar APIs
`Configure/Register/Answer/Hangup/Hold/Mute/Transfer` com limpeza de
credenciais, validação e diagnósticos sem segredos.

**Estágio 3:** teste funcional com central Dígitro e headset físico, duas
edições lado a lado, evolução do monitor SIP, tema e paridade da UI WinUI 3.
Nenhuma edição pronta para produção até esse aceite.

## Critérios de aceite da primeira entrega

1. `AXEVoice` Win32 existente compila e passa CI inalterado funcionalmente.
2. Projeto WinUI 3 abre como app Windows separado; teclado 0–9, *, #,
   apagar e navegação básica são operáveis; layout não usa barra de rolagem.
3. O WinUI 3 apresenta **"SIP indisponível"** sem a bridge do backend real;
   chamada e recepção permanecem desabilitadas.
4. `axv_frontend_bridge.dll` exporta ABI pequena, versionada, com projeção
   de estado compartilhada e testes nativos de segurança da sessão.
5. Build e artefatos independentes no GitHub Actions para Win32 e WinUI 3.
6. Documentar requisitos de runtime WinUI 3 e distinguir protótipo de UI
   do candidato nativo funcional.

## Rejeições

Não converter o app Win32 em WebView/Electron, não reimplementar SIP em C#,
não criar backend HTTP local para comandos telefônicos, não colocar segredos
nos logs, não adicionar estados verdes simulados e não marcar o WinUI 3
como softphone funcional antes do próximo estágio.
