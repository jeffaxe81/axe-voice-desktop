# AXE Voice v0.66 — controle de registro SIP com thread nativa proprietária

Base: v0.65 / PR #81 (64/64 testes Windows). Código portado do experimento isolado PR #70, conciliado com a versão mais nova do adaptador de callbacks SIP.

## Entregue

1. `BaresipNativeRegistrationHost` encapsula um `BaresipNativeLibreReactor`, uma fila de comandos sem ponteiros e um `BaresipSdkRegistrationGateway`. O `re_main()`, os `bevent`, `ua_register()`, `ua_unregister()`, o watchdog e a limpeza rodam todos na thread proprietária.
2. O pedido de registrar atravessa a fila com geração opaca, apenas com prazo de expiração; a thread da interface **não** recebe `ua*`, senha, endereço SIP ou callback C.
3. O resultado `Submitted` significa apenas que `ua_register` foi solicitado; `Confirmed` requer que o adaptador tenha recebido `BEVENT_REGISTER_OK` e que `ua_isregistered(selected)` ainda confirme o registro. A correção v0.65 mantém o lease de callback até a parada encerrar.
4. Cancelamento, expiração do prazo, erro de inicialização, `BEVENT_SHUTDOWN`, falha de despertar da fila e reaproveitamento de geração antiga encerram ou bloqueiam o pedido (*fail closed*).
5. Novo `libre_registration_owner_integration`: CTest usa threads C++ reais e fakes C de libre e Baresip, com contadores de registros, cancelamentos, callbacks e limpeza de recursos.

## Limites expressos

O controlador é um módulo interno de plano de controle, integrado à árvore do AXE Voice **apenas por CTest**. Não está vinculado ao executável GUI `AXE-Voice.exe`, que permanece com `UnavailableSipEngine`. Não cria nem autentica uma conta UA nativa real, não lê credenciais, não estabelece chamadas, não conecta RTP e **não emite `VerifiedNativeApi` para a interface**. Não interpretar um CTest com SDK fake como um registro SIP de produção.

## Bloqueadores antes de passar para operação real

- Auditar e fixar os artefatos e cabeçalhos do Baresip/libre por versão/hash e validar compatibilidade de ABI real.
- Integrar provisionamento de conta SIP, bootstrap e término de vida da UA no próprio loop nativo, protegendo credenciais.
- Criar canal de observações autenticadas, com novas gerações e prova do estado nativo, até a UI sem acesso direto a ponteiros.
- Executar chamadas de entrada/saída e áudio bidirecional com PABX real, headset USB/Bluetooth, rede e autenticação, TLS e SRTP.
- Somente fechar PR Draft e promover para release após CI, testes reais e evidências de homologação.

## Cadeia de releases

v0.63 (fencing da troca de áudio) → v0.64 (libre + primitivas) → v0.65 (lease dos callbacks + confirmação cruzada) → **v0.66 (registro nativo orquestrado/testado isoladamente)**.
