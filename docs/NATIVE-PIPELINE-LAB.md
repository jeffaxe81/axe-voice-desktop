# AXE Voice v0.38 — Prova sintética do pipeline de eventos

Escopo: teste integrado `test_baresip_native_pipeline` ligando
`BaresipNativeIngress` → `BaresipEventMailbox` → `SipEventProjection`,
sem Baresip nativo, contas SIP, rede ou mídias reais.

## Cobertura

- A pré-admissão sozinha **não** autoriza a discagem: evidência `None`
  mantém SIP indisponível, mesmo após eventos de registro sintéticos.
- Eventos sintéticos sob prova **injetada pelo teste** exercitam registro,
  toque, conexão, encerramento, ações permitidas e tokens monotônicos.
- Divergência de identidade revoga a geração e descarta eventos pendentes.
- Lote forjado maior que a capacidade é recusado sem leitura além do array;
  a geração rejeitada não pode ser reabilitada.
- Encerramento concorrente com callbacks tardios remove a sessão e eventos.
  Trata-se de teste de corrida de controle, **não** de homologação do Baresip.

## Barreiras para integrar o Baresip real

1. Fixar versões/hashes e revisar licenças de Baresip, libre/re e codecs.
2. Criar adaptador nativo in-process para `bevent_h`, com fonte de UA,
   conta e identidade de chamada verificadas; nunca confiar em stdout/log.
3. Garantir remoção dos callbacks antes de `reset_after_teardown()`.
4. Só promover `SipEvidence::VerifiedNativeApi` após atestação local
   do adaptador auditado, com geração corrente e ciclo de vida validado.
5. Validar Windows 10/11, registro SIP, entrada/saída, microfone/fone,
   áudio bidirecional, TLS/SRTP e recuperação em falha.

O executável segue com `UnavailableSipEngine`, comandos de chamada
bloqueados e **sem SIP ou áudio operacional**. Não liberar produção nem
mesclar na `main` antes de homologação. PR empilhado sobre #15.
