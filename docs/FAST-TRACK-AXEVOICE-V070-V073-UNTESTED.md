# AXE Voice — quatro entregas antecipadas sem testes novos (v0.70–v0.73)

Por solicitação, estas alterações foram **publicadas sem adicionar testes e sem executar/aguardar testes manualmente nesta rodada**. Os workflows automáticos existentes do GitHub podem executar por causa dos pushes. Nenhum resultado de aprovação de CI é pressuposto ou anunciado. Todas as PRs permanecem Draft, empilhadas, sem merge na main.

| Versão | PR | Entrega de código | Conexão ao softphone |
|---|---|---|---|
| 0.70 | #86 | `src/baresip_sdk_core_bootstrap.h`: preparo opcional do SDK nativo (conf, core, UA, módulos) e limpeza reversa | Módulo isolado; não habilitado |
| 0.71 | #87 | `src/sip_credential_lease_win32.h`: leitura efêmera pelo Gerenciador de Credenciais e limpeza imediata de memória | Módulo isolado; não escreve senhas |
| 0.72 | #88 | `src/native_registration_diagnostics.h`: diagnóstico sem ponteiros/credenciais, gerações SIP e estado conservador | Não emite `VerifiedNativeApi` |
| 0.73 | #89 | `src/native_sip_release_gate.h`: barreira de ativação comercial com requisitos de segurança/homologação; macro ausente por padrão | O binário GUI segue desabilitado para SIP real |

## Regras de segurança

- Não foi adicionada senha ao código, AOR, fila, log ou snapshot.
- Nenhuma mudança liga `AXE-Voice.exe` ao SDK nativo ou libera chamadas RTP: `UnavailableSipEngine` permanece intocado.
- O caminho de SDK real da v0.70 exige auditoria e opt-in explícito; ainda não foi compilado contra SDK pinado/verificado.
- O acesso aos segredos na v0.71 deve passar por revisão de memória/credenciais no Windows antes de ligar à autenticação real.
- O status `CallbackConfirmedUnverified` na v0.72 **nunca** significa SIP registrado/autenticado.
- O gate da v0.73 retorna `DisabledInLabBuild` nas compilações atuais; qualquer opt-in futuro depende de revisão formal e provas de autenticação, TLS, SRTP, PABX e áudio bidirecional.

## Pendências para homologação

1. Integrar e compilar SDK Baresip/libre real auditado com versões/hashes aprovados e módulos seguros.
2. Conferir armazenamento e rotação de segredos do Windows, autenticação SIP positiva/negativa e erros TLS.
3. Testar registro, chamadas entrantes/saintes, RTP/SRTP, áudio PC/headset e mudança de dispositivo em PABX real.
4. Executar os testes automáticos e de caixa-preta, registrar CI e evidências, corrigir regressões e só então retirar PRs de Draft/considerar merge.
