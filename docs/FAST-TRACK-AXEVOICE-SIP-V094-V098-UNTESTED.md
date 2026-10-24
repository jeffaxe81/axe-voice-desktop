# AXE Voice — cinco entregas v0.94 a v0.98 (não homologadas)

Data do ciclo: 24/10/2026. Partida em v0.93. **Nenhum teste ou build foi executado manualmente** por instrução anterior; automações do GitHub podem ser disparadas separadamente. Revisão estática não prova compilação, autenticação ou eficácia.

| Versão | Implementação publicada | Limites reais |
|---|---|---|
| **0.94** | Observação de `BEVENT_REGISTER_FAIL` atribuída à UA e sessão proprietárias; gateway encerra a tentativa e devolve `RegistrarRejected` sem aguardar expirar o timeout. Encadeamento ao dispatcher e diagnóstico. | Callback nativo ainda precisa de SDK auditado e homologação. Rejeição não contém segredo nem prova de autenticação. |
| **0.95** | `BaresipNativeRegistrationHost` revoga `accepting_` em cancelamento, rejeição do servidor, timeout, overflow, falha de execução, perda de geração e thread incorreta. | Não encerra automaticamente o processo no frontend; a aplicação integradora precisa executar `stop()` e aguardar a thread. |
| **0.96** | `ExperimentalNativeRegistrationController::poll_control_plane()` consulta o resultado nativo e atualiza o orçamento de falhas **uma única vez por tentativa/sessão**; distingue pendente, callback não verificado, falha terminal e cancelamento pelo usuário. | Sem retentativa automática. Um callback `REGISTER_OK` ainda não comprova autenticidade/TLS/SRTP: `sip_authenticated` e `media_verified` seguem `false`. |
| **0.97** | Consulta ao cofre Windows distingue ausência verdadeira, permissão negada, falha do sistema e registro inválido, validando tipo, alvo e tamanho do segredo. | Somente metadados são exibidos; `CredReadW` retorna segredo em memória de processo, limpo com `SecureZeroMemory`, mas ainda não há integração segura de credenciais ao SDK. |
| **0.98** | Menu MAIS e **F6**: checklist de conta/TLS, credencial local, integração Baresip/re, autenticação PABX, RTP/SRTP, áudio e homologação; sem revelar identidade ou segredo. Versões da GUI, diagnóstico e CMake atualizadas para 0.98.0. | Checklist informativo, não inicializa motor, não registra ramal e não libera chamadas. |

## Segurança e implantação

- O alvo nativo `axv_native_registration_lab` continua **OFF por padrão**, isolado de `AXEVoice.exe`; SDKs Baresip e libre exigem verificação de hashes e ABI.
- O frontend segue usando `UnavailableSipEngine`, `SipEvidence::None` e validação `authorize_dial`. Nenhuma simulação de registro pode ativar o botão CHAMAR.
- Para homologação futura: compilação MSVC, testes de eventos `REGISTER_FAIL`, cancelamento e corrida de thread; credenciais corretas/incorretas no SDK; validação do certificado TLS; SRTP e áudio real entre headset/PC; e testes de caixa preta, integração, operação e segurança com PABX.
- Não declarar produção, não alterar `main` sem evidências reais de homologação.

**Revisão de implementação nesta rodada:** mudanças inspecionadas visualmente e por comparação textual; nenhum teste, build, assinatura, publicação de instalador, chamada VoIP ou homologação física foi realizado.
