# Roadmap atual da main

A main recebeu os sete PRs de laboratório em25/10/2026. Consulte [situação, evidências e próxima entrega](AXEVOICE-RELEASE-PROMOTION.md) e [resolução dos conflitos](integration/OPEN-PRS-RESOLUTION.md).

## Candidato nativo — próximas releases

- [x] SDK auditado, registro/TLS local, GUI e instalador candidato (PR #132).
- [x] 0.52: seleções de microfone/voz aplicadas ao WASAPI; rota fixada por sessão; bloqueio antes de chamar/atender se o endpoint faltar. [Escopo e validação](integration/NATIVE-AUDIO-ROUTING.md).
- [x] 0.53: mudo do microfone e pedidos de espera/retomada locais, com leitura real do SDK e comandos vinculados à chamada exibida. [Escopo e limites](integration/NATIVE-MUTE-HOLD.md).
- [x] 0.54: aviso antigo de áudio corrigido; transferência direta com revisão do destino, uma tentativa por chamada e falha lida do SDK. [Escopo e limites](integration/NATIVE-TRANSFER.md).
- [x] 0.55: toque local de chamadas recebidas verificadas, saída independente e cancelamento por chamada. [Escopo e limites](integration/NATIVE-RINGTONE.md).
- [x] 0.55.1: diagnóstico F2 separa disponibilidade de discagem e homologação física.
- [x] 0.56: cópia local de diagnóstico sem dados da conta. [Escopo e limites](integration/NATIVE-DIAGNOSTIC-COPY.md).
- [x] 0.57: modo explícito DTMF via SIP INFO, vinculado à chamada e bloqueado em espera/transferência. [Escopo e limites](integration/NATIVE-DTMF.md).
- [x] 0.57.1: recuperação do toque, rolagem por foco e integridade do pacote após auditoria em cinco frentes. [Escopo](integration/PARALLEL-HARDENING.md).
- [x] 0.58: duração local de chamada estabelecida, gate de versões e handshake TLS local com timeout separado. [Escopo e limites](integration/NATIVE-CALL-DURATION.md).
- [x] 0.59: inventário de áudio assíncrono em duas abas, coalescência, cancelamento visual e recuperação, conforme requisitos registrados de PR30. [Escopo](integration/ASYNC-AUDIO-INVENTORY.md).
- [x] 0.60: observador Core Audio, debounce, fallback periódico e invalidação de inventário obsoleto. [Escopo](integration/AUDIO-DEVICE-NOTIFICATIONS.md).
- [ ] Homologar registro/chamadas na Dígitro e áudio bidirecional com headset físico.
- [ ] Validar troca de dispositivos durante chamada antes de permitir roteamento ao vivo.
- [ ] Implementar SRTP se exigido pela implantação; TLS de SIP não criptografa RTP.

A homologação na Dígitro e no headset foi mantida pendente por solicitação do usuário em25/10/2026.

A build padrão segue sem SDK; o instalador nativo continua candidato. Os gates automatizados de cada PR não substituem aceite da central/hardware. O roteiro abaixo pertence à fundação histórica MicroSIP.

---

# Roadmap

## P1.1 — Fundação
- [x] Criar repositório GitHub
- [x] Criar README inicial
- [x] Criar branch develop
- [x] Completar documentação-base
- [x] Validar estrutura
- [x] Consolidar checkpoint

## P1.2 — Baseline MicroSIP
- [x] Identificar versão oficial
- [x] Registrar origem, versão e data
- [x] Inventariar licenças iniciais
- [x] Obter archive oficial e registrar SHA-256
- [x] Importar source sem customização
- [x] Registrar manifesto de arquivos
- [x] Inventariar dependências iniciais
- [ ] Criar tag upstream

Baseline: **MicroSIP 3.22.16.0**  
SHA-256: `9c1ff942ae1ae56bdfcdd04c3d6e242a056d7f0dd50bb19f6e13226e7c819b83`

## P1.3 — Build original
- [x] Identificar estrutura esperada do PJSIP
- [x] Identificar plataformas/configurações do projeto
- [x] Definir primeiro alvo: Release x64
- [ ] Fixar baseline PJSIP/build dependencies
- [ ] Criar workspace de build reproduzível
- [ ] Reproduzir dependências
- [ ] Compilar sem alterações
- [ ] Executar
- [ ] Registrar conta SIP
- [ ] Testar chamada originada/recebida

## P1.4 — White label
- [ ] Criar working tree derivada da baseline
- [ ] Nome
- [ ] Ícones
- [ ] Branding
- [ ] Metadados do executável
- [ ] Instalador

## Projeto 2 — Mobile
Registrado para etapa posterior. Não iniciar antes de liberação formal do Projeto 1.

Consulte a [matriz das funcionalidades restantes](integration/REMAINING-FUNCTIONALITIES.md) para separar código entregue, escopo histórico e homologação externa.

- Candidate0.61: intenção SIP/SIPS preservada, limites de entrada nativa e integridade literal de arquivos no pacote; quatro frentes de testes de segurança. [Registro e acompanhamento](integration/SECURITY-HARDENING.md).
