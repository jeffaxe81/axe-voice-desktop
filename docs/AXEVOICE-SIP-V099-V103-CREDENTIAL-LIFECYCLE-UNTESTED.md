# AXE Voice Desktop — ciclo experimental v0.99–v0.103

Data: 24/10/2026. Continuação da v0.98, **sem compilar, executar testes ou homologar Windows/PABX nesta rodada**, conforme orientação anterior. Uma revisão textual/estrutural não equivale a teste.

## Cinco entregas de implementação

| Versão | Evolução | Limites |
|---|---|---|
| v0.99 | A leitura de credenciais no `sip_credential_lease_win32.h` verifica tipo `CRED_TYPE_GENERIC` e nome-alvo exato antes de entregar o blob efêmero a um consumidor. O blob é limpo com `SecureZeroMemory` na saída por RAII. | Não autentica servidor SIP. A limpeza desta cópia não elimina automaticamente outras cópias que um SDK possa criar. |
| v0.100 | `sip_native_owner_credential_bridge.h` deriva um identificador do cofre da identidade DNS/usuário validada e cria hooks de instalação/revogação executados somente na thread `re_main`. A vida do blob fica limitada ao callback. | O callback do SDK ainda não foi implementado/auditado para uma versão real de Baresip. |
| v0.101 | A factory opcional `make_experimental_sdk_registration_host` recusa criar host nativo sem **ambos** os callbacks `install` e `revoke`; `make_core_gated_native_ua_hooks` instala a credencial após preparar o core, antes de criar UA, e revoga no rollback e no desligamento, antes de liberar o core. | Os callbacks exigem adaptador externo auditado que instale e limpe segredo corretamente; não há credencial adicionada ao AOR ou ao frontend. |
| v0.102 | Controlador experimental diferencia `CancellationRequested` da confirmação `UserCancelled`. Solicitações e callbacks posteriores ficam sem efeito na mesma geração após cancelamento/falha. O cancelamento do usuário não é contado como falha de senha. | Não há reconexão automática e a autenticação real continua sem prova. |
| v0.103 | `MAIS > Remover credencial SIP do Windows` (F7) solicita confirmação explícita antes de executar `CredDeleteW` para o usuário e servidor nos campos atuais. Exibe resultado específico sem revelar senha. O projeto CMake e a interface passam a 0.103.0. | A remoção é permanente no cofre do usuário; o usuário precisa recriar a credencial no Windows caso deseje utilizá-la futuramente. |

## Limites de segurança e estado real

- A lib `axv_native_registration_lab` continua opcional/OFF por padrão e agora depende de `advapi32` para chamadas ao cofre no Windows.
- **Nenhuma implementação real de `NativeSdkCredentialPort::install` / `revoke` foi ligada a um SDK Baresip auditado.** O integrador deve comprovar ABI, configuração real de autenticação de challenge SIP, ciclo de vida e limpeza de cópias em memória. Retornar `true` artificialmente nunca equivale a autenticação.
- Os hooks são callbacks que devem executar somente no owner thread, e o SDK não pode manter ponteiro para o buffer após o retorno da função. Se o SDK mantiver uma cópia, o revoker precisa eliminá-la; isto exige teste e revisão.
- A GUI `AXEVoice.exe` mantém `UnavailableSipEngine` e `SipEvidence::None`. Nem presença de credencial, nem execução do processo, nem callback `REGISTER_OK` sintético habilitam chamadas.
- Continuam pendentes: build MSVC Windows 10/11; testes de ciclo de vida e cancelamento; auditoria de hashes/ABI e licenciamentos Baresip/re; integrações reais com autenticação SIP, TLS peer validation, SRTP, dispositivo de áudio, chamadas bidirecionais e aceitação em PABX/headset.
- `main` não deve receber esta sequência antes da homologação e do GO de segurança.

## Cuidados de operador

- F4 exibe alias de credencial apenas mediante ação explícita; F5 abre o Gerenciador de Credenciais; F7 executa **remoção irreversível** apenas se o usuário confirmar.
- F7 remove apenas a credencial associada ao par de campos `Servidor SIP` e `Usuário SIP` no momento do clique. Não lista nem apaga outras credenciais.
- Nenhum usuário deve colar ou compartilhar credenciais SIP em issues, logs, prints ou relatórios.

## Estado da entrega

Código registrado, diff inspecionado; **nenhuma compilação, execução de testes, CI manual, assinatura de instalador, chamada real ou homologação física comprovadas neste ciclo**.
