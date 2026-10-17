# AXE Voice — transição ao produto proprietário (fase Áudio 0.5.0)

**Decisão:** A primeira entrega do novo produto AXE Voice será **exclusivamente de áudio**, com aplicação Windows desktop e interface completa/compacta/flutuante. **Vídeo, chat, Web e mobile não integram a primeira entrega.**

## Separação legal e técnica

Este repositório público contém uma baseline histórica do **MicroSIP 3.22.16.0 (GPLv2)**, destinada à avaliação técnica/rastreabilidade. **Não utilizar código-fonte, recursos ou implementações do MicroSIP no AXE Voice proprietário**. Essa baseline não deve ser empacotada no produto comercial de código fechado.

O AXE Voice utiliza código novo da Axesistemas e um motor externo **Baresip/libre** (licença BSD conforme upstream), sujeito à auditoria das dependências, codecs e licenças. O código proprietário **não será incluído neste repositório público**; o desenvolvimento deve prosseguir em repositório privado com acesso controlado.

## Escopo da entrega de áudio v0.5.0 (developer preview)

- GUI nativa Windows (Win32): normal, mini, fixar no topo e área de notificação.
- Bridge cliente Baresip `ctrl_tcp`: chamada de saída, atender, encerrar, mudo e espera.
- Complementos no código independente: comando DTMF `sndcode`, transferência simples `transfer`, correlação de respostas por token, validação estrita de parâmetros e indicação de estado confirmada pelo motor.
- Testes portáveis no Linux: 4 suítes aprovadas (protocolo, comandos de áudio, estados da UI, bridge simulada). **Não equivalem a testes de telefonia real.**

## Bloqueadores de homologação comercial

1. Compilar e rodar o aplicativo no Windows 10/11 x64; concluir CI Windows.
2. Fixar e empacotar versões aprovadas de Baresip/libre e codecs, com SBOM e avisos legais.
3. Substituir/fortalecer o atual `ctrl_tcp` em loopback, que não autentica clientes locais, antes de distribuição. `127.0.0.1` não resolve o problema de outros processos locais.
4. Proteger credenciais SIP via mecanismos seguros do Windows, sem registrar segredos em logs.
5. Validar registro SIP, áudio RTP bidirecional, mute/hold, DTMF, transferência, TLS/SRTP, chamada de saída/entrada em PABX real.
6. Criar instalador assinado, requisitos mínimos e política de atualização.

## Expansão futura (sem código no primeiro pacote)

- **Vídeo:** adaptar módulos de vídeo do Baresip e a experiência de câmera, após compatibilidade e licença.
- **Chat:** integrar cliente XMPP (libstrophe candidata) e servidor Prosody, depois de confirmar produto, arquitetura de autenticação, histórico e requisitos de segurança.
- **Multiplataforma:** Web via solução própria compatível com browser e mobile com Flutter como candidata de UI.

**Situação:** a v0.5.0 permanece uma *prévia de desenvolvimento*, não está pronta para produção nem comercialização. Não há executável Windows homologado nesta etapa.

## Incremento privado v0.6.0 — 17/10/2026

A prévia proprietária de áudio evoluiu após este PR documental (sem incorporar MicroSIP nem publicar código privado): parser JSON robusto e estrito, CLI com validação estruturada dos tokens, correlação de chamadas por ID, expiração de comandos pendentes em 6 segundos, regras de comando por estado SIP, UI Mini reduzida a 352×395 px, prevenção de segunda instância e workflow de build Windows para **repositório privado**.

**Evidência do ambiente de desenvolvimento:** 6/6 suítes aprovadas em Linux Release e também sob AddressSanitizer/UndefinedBehaviorSanitizer, incluindo testes mock de protocolo. **Não houve compilação/execução GUI em Windows ou telefonia RTP real.** O pacote de código v0.6.0 deve permanecer privado, separado deste repositório público com a baseline GPL MicroSIP.

**Próximos critérios de aceite:** compilação Windows x64, Baresip/libre empacotados e revisados, IPC autenticado, credenciais seguras, testes SIP/RTP e instalação assinada. Chat e vídeo permanecem fora da primeira entrega.

## Checkpoint v0.7.0 — segurança de estado (17/10/2026)

O pacote **privado** de desenvolvimento v0.7.0 evoluiu a interface e a lógica de áudio sem levar código proprietário a este repositório histórico. Melhorias: eventos `class=call` exigem ID correlacionado, transições duplicadas são ignoradas, cronômetro da chamada baseado em relógio monotônico, contador em memória de chamadas recebidas não atendidas (excluindo rejeições locais), e comandos conflitantes ficam bloqueados durante confirmação. Inclui pré-check read-only para detectar `ctrl_tcp` exposto fora de `127.0.0.1`.

**Testes:** 8/8 suítes em Linux Release, 8/8 sob sanitizadores ASan/UBSan. São testes sintéticos de protocolo e máquina de estados, **não homologação Windows/SIP/RTP real**. A versão Windows, o motor externo Baresip e o canal IPC autenticado continuam pendências impeditivas de comercialização. **Vídeo e chat não fazem parte da entrega inicial.**

## Checkpoint privado v0.8.0 — 17/10/2026

Entrega apenas de **código-fonte do audio desktop**: proteção contra discagem/atendimento/encerramento duplicados enquanto o motor SIP não confirma a transição; validação estrita de destino SIP com domínio e porta; preferência de modo Mini e janela sobreposta persistida no Registro HKCU (código Win32); filtragem de eventos de registro pela classe UA. Acrescentado script de validação laboratorial Windows, sem iniciar chamadas automaticamente.

**Evidências obtidas somente em Linux:** 10/10 suítes no CMake Release e 10/10 no clang++ com AddressSanitizer/UndefinedBehaviorSanitizer. O mock valida JSON/netstring, não SIP/RTP. **Não houve compilação Windows nem chamadas SIP/RTP reais neste checkpoint.** A conexão `ctrl_tcp` permanece sem autenticação e é bloqueador de produção. Código privado e ZIP não estão publicados neste repositório público MicroSIP GPL. Vídeo e chat seguem fora da primeira versão.
