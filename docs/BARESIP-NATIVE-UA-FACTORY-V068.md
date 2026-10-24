# AXE Voice v0.68 — ponte nativa Baresip para criação e liberação de conta

Base: v0.67 / PR #83. Artefato padrão continua sendo LAB sem SIP funcional.

## Integração de código

- `src/baresip_native_ua_factory.h` implementa `BaresipNativeUaFactory::make_owner_hooks`, ponte para as funções públicas do Baresip `ua_alloc(struct ua**, const char*)` e `ua_destroy(struct ua*)`.
- O contrato de hooks da v0.67 invoca ambas **exclusivamente na thread proprietária** do `re_main()`; a destruição ocorre depois da parada do gateway, do unregister e da barreira de callbacks.
- `NativeUaIdentity` contém somente identificador de ramal, hostname DNS ASCII, porta e política de transporte. Não há campo de senha. O construtor de AOR é estrito: limite de comprimento, formato DNS válido, caracteres SIP seguros, sem URI de usuário fornecida integralmente, sem parâmetros extras e **TLS obrigatório**. Exemplo de AOR aceita: `<sip:1001@pbx.example.com:5061;transport=tls>`.
- Falha de validação bloqueia a chamada à API nativa. `ua_alloc()` recusada ou com ponteiro parcial aciona rejeição/rollback; reinício não herda uma UA antiga.
- `tests/baresip_sdk_abi_probe.cpp` agora exige a assinatura `int ua_alloc(ua**, const char*)` e `unsigned ua_destroy(ua*)` e verifica os símbolos na ligação. Quando o SDK opcional é configurado, CMake usa o hash esperado do cabeçalho e da biblioteca já existentes para validar procedência.
- Novo CTest `libre_registration_native_sdk_ua_factory`: fluxo fim-a-fim pelo `BaresipNativeRegistrationHost` com threads C++ reais e **fakes** Baresip/libre. Exercita criação, registro simulado, parada, destruição, nova geração, rejeição de entrada insegura, falha de `ua_alloc()` e rollback de resultado parcial.

## Controles e bloqueadores

1. A ligação padrão `AXE-Voice.exe` **não** inclui a factory nativa; nenhum registro de ramal, RTP ou chamada real foi habilitado.
2. Um `libre_init()` isolado não é inicialização suficiente de todo o core do Baresip. Antes do primeiro `ua_alloc()` real, é obrigatório inicializar o core da biblioteca e carregar módulos, TLS, configurações e lifecycle auditados no **mesmo contexto nativo**.
3. Esta etapa **não implementa autenticação por senha**, cofre de credenciais, certificado TLS validado, SRTP, chamada real, mídia USB/Bluetooth nem proveniência `VerifiedNativeApi` na interface. Um AOR contendo `transport=tls` é solicitação de transporte, **não prova de TLS**.
4. Nunca criar AOR por concatenação de entrada arbitrária ou colocar senhas no AOR, repositório, logs, fila da UI ou no snapshot.
5. Para aprovação comercial: SDK real pinado por hashes e ABI, bootstrap Baresip, provedor de credenciais protegido, registro em PABX com sucesso/falha, chamadas bidirecionais, testes de áudio e segurança TLS/SRTP.

## Critério de aceitação da v0.68

CI Windows MSVC Release verde com testes unitários, caixa-preta e integração; nova factory com falhas de SDK exercitadas por mocks; nenhuma regressão do frontend ou do binário LAB. **Não** significa homologação de SIP real.
