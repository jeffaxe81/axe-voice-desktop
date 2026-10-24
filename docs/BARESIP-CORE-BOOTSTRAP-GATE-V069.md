# AXE Voice v0.69 — barreira de inicialização do núcleo Baresip

Base: v0.68 / PR #84, com a integração `ua_alloc`/`ua_destroy` testada usando Baresip/libre simulados.

## Implementação no ramo

O novo arquivo `src/baresip_native_core_bootstrap.h` acrescenta a composição `make_core_gated_native_ua_hooks`. Esse contrato recebe:
- `prepare()`: callback de inicialização do núcleo Baresip, chamado **na thread proprietária do `re_main`**, que só deve retornar `true` após configuração do núcleo, inicialização do subsistema UA e política TLS aprovada.
- `release()`: callback **obrigatório, sem exceção**, responsável por desmontar inclusive uma configuração iniciada parcialmente, depois da liberação da UA.

A factory da v0.68 só invoca `ua_alloc()` se a fase de preparação retornar êxito. Rejeição, exceção, ausência de `prepare/release` ou falha de `ua_alloc()` causam recusa de provisionamento e rollback de núcleo. A liberação ocorre em ordem estrita: gateway de registro/callbacks → `ua_destroy` → `release()`. Uma nova geração só usa uma conta nova.

O novo CTest `libre_registration_core_bootstrap_gate` executa 3 cenários de falha de inicialização, falha de alocação, registro fake e reinício, tudo com **funções falsas para o núcleo e o SDK**. Os testes anteriores do softphone Windows permanecem intactos.

## Por que isto ainda não é autenticação SIP real

A documentação oficial de uso de libbaresip mostra, antes de criar uma conta, uma sequência que inclui inicializar libre, configurar a biblioteca, iniciar o núcleo Baresip, iniciar o subsistema UA e carregar módulos. O núcleo também requer configuração de TLS, certificados e mídias. A ordem e as assinaturas devem ser validadas contra uma versão/hashes específicos da dependência.

Fontes de referência:
- https://github.com/baresip/baresip/wiki/Using-baresip-as-a-library
- https://github.com/baresip/baresip/blob/main/src/main.c

**A v0.69 só implementa o contrato de ciclo de vida e a falha segura de inicialização.** Não chama `baresip_init()` ou `ua_init()` na execução real; não inclui provedor de senha; não verifica certificado TLS, não usa SRTP e não executa SIP/RTP contra um PABX. O binário `AXE-Voice.exe` continua em modo LAB, com `UnavailableSipEngine`, sem chamadas nem `VerifiedNativeApi` para a interface.

## Próximas pendências obrigatórias

1. Selecionar e auditar uma versão Baresip/libre, com hashes e testes de ABI do núcleo e dos módulos.
2. Implementar `prepare()`/`release()` com as chamadas reais do SDK, configuração restrita, validação TLS e ordem reversa de cleanup.
3. Usar um provedor protegido de credenciais que não transporte segredos via janela, AOR, logs, mailbox nem snapshots. Nenhum segredo foi persistido nesta versão.
4. Testar em PABX real registro correto e senha incorreta, cancelamento, falha de DNS/TLS, reconexões, chamadas em ambos os sentidos, codecs, áudio PC/headset e SRTP.
5. Manter a PR Draft até CI verde e homologação documentada. Não confundir CTest com funcionamento comercial.
