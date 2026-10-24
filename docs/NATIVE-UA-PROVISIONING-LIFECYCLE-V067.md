# AXE Voice v0.67 — ciclo de vida protegido da conta SIP nativa

Base: v0.66 / PR #82 (65/65 testes automatizados aprovados).

## Implementação

- `BaresipNativeRegistrationHost::NativeUaLifecycleHooks` fornece um contrato de **criação e liberação de UA exclusivamente na thread proprietária** de `libre/re_main`.
- O provedor real de conta será responsável por carregar/validar credenciais e criar uma UA com o SDK auditado. A estrutura desta versão recebe apenas `ua*` **dentro** da thread proprietária; senhas, AOR e URIs não são transportados por `submit`, `cancel` nem `snapshot`.
- O provedor é chamado **uma vez por geração**, antes de o host aceitar comandos SIP. `create()` ausente, par incompleto, retorno nulo e exceção rejeitam o início (fail closed), sem anunciar conta ativa.
- A UA permanece estável durante `dispatcher.tick`. No desligamento, o gateway executa `stop()`, desregistra callbacks e aguarda quiescência; **somente depois** o hook `destroy(ua*)` pode liberar a conta na mesma thread.
- `snapshot().account_provisioned` indica somente existência da conta nativa naquela geração. **Não significa registro SIP ou mídia autenticada**, nem carrega credenciais/identificadores.
- O modo anterior de resolver UA continua aceito para testes de migração isolados. Em operações futuras, priorizar o modo gerenciado.
- Novo CTest `libre_registration_owned_ua_lifecycle`: usa `--owned-ua` no fixture do fluxo Baresip/libre simulado, com threads reais, confirmação síncrona de registro, liberação após unregister, falhas de provisionamento, exceções, reinício e bloqueio de hooks incompletos.

## Contrato de segurança e pontos de revisão

1. `create` e `destroy` são invocados no loop nativo. `destroy` não deve lançar exceção nem chamar `stop`, e precisa manter a UA viva até a parada do gateway.
2. Não há armazenamento seguro de senha, criação real de UA via `ua_alloc`, TLS, SRTP ou conexão a PABX nesta mudança.
3. A interface `AXE-Voice.exe` permanece ligada ao motor indisponível (fail closed). O módulo está integrado **apenas aos testes CTest**, não ao softphone funcional.
4. SDK real deve ser pinado por SHA, receber configuração de conta com regras de segurança e passar testes em PABX e áudio bidirecional antes de habilitar o registro na UI.
5. `account_provisioned` nunca deve ser traduzido para `SIP registrado`. Somente callbacks nativos validados + `ua_isregistered` podem informar o estado de registro ao controlador; a interface ainda exige adaptador de evidência separado e autenticado.

## Próxima entrega

Conectar o *bootstrap* da conta ao SDK Baresip real, com memória de credenciais protegida e política TLS/SRTP; adicionar testes de falha de autenticação e conta removida durante sessão; homologar chamadas em PABX real e headset USB/Bluetooth.
