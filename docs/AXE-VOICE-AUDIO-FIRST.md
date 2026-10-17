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
