# AXE Voice — serviço C++ de aplicação compartilhado (estágio 2A)

**Data:** 2026-11-07
**Origem:** `docs/architecture/DUAL-FRONTENDS.md` e estágio seguinte aprovado.
**Objetivo:** sair do código Win32 monolítico aos poucos, preservando chamadas reais.

## Arquitetura

1. Adicionar `VoiceApplicationService<NativePort>`, uma fachada C++17 independente do Windows e do SDK Baresip.
2. Consumir observações `SipRuntimeObservation` verificadas da porta nativa; oferecer snapshot imutável e enviar `Dial/Answer/Hangup` somente quando sessão, chamada e estado correspondem aos que o usuário viu.
3. A porta real será `BaresipSipEngine`; ela continua proprietária da thread/SDK e **faz a autorização final novamente na fila nativa**. O serviço não transporta, armazena ou expõe a senha.
4. Integrar `Win32` pela mesma fachada para leitura de status, discagem, atender e encerrar, sem alterar o projeto visual, codecs, áudio ou a seleção da conta.
5. O `WinUI 3` continua sem backend ativo neste subestágio. No 2B, uma instância única da fachada e do motor deve viver na DLL nativa do próprio processo WinUI 3, com ABI C versionada, com controles testados e credenciais somente protegidas no Windows.

## Regras

- Nunca considerar processo ativo como registro SIP comprovado.
- Discagem requer `Registered` atual, token de chamada zero, destino sintaticamente válido e uma nova autorização no motor.
- Atender requer `Ringing` e mesma sessão + `call_id` da interface e do motor.
- Encerrar requer `Ringing/Calling/InCall` e mesma sessão + `call_id` da interface e do motor.
- Um snapshot antigo não pode acionar comandos em uma nova sessão.
- Um comando enfileirado não significa chamada conectada nem mídia bidirecional.
- Não criar segundo Baresip, backend HTTP ou simulação de chamadas reais.
- Manter Win32 build sem SDK nativo e seus controles intactos.
- WinUI 3 continua no modo prévia/offline até conexão e homologação.

## Critérios de aceite

- Testes de 2.000 cenários de chamadas atuais e 2.000 obsoletas (simulação)
  mais falha de submissão nativa, sem chamadas de rede.
- Win32 CTest e testes de stress do reactor todos aprovados.
- WinUI 3 CI permanece verde e não habilita o botão Ligar sem engine.
- SDK Baresip testado, incluindo loopback e empacotamento nativo.
- Merge na `main` apenas com todos os pipelines necessários aprovados.

**Fora de escopo deste subestágio:** expor credenciais e comandos C ABI para C#, migrar runtime Baresip para DLL, adicionar SIP real ao WinUI 3, ou declarar homologação PABX.
