# AXE Voice LAB — ciclo SIP v0.84 a v0.88 (sem testes)

Implementado em 24/10/2026, partindo do checkpoint v0.83. **Não foram criados ou executados testes, não houve build Windows nem prova de registro SIP nesta rodada.** Os workflows automáticos do GitHub podem ser disparados por pushes.

## Cinco entregas de código

| Versão | Nova entrega | O que realmente funciona |
|---|---|---|
| 0.84 | Política de pré-ativação TLS | `src/sip_native_preflight.h` exige sintaxe de conta válida, identidade DNS/ASCII compatível com UA Baresip e seleção de TLS. Apenas avaliação offline. |
| 0.85 | Build opcional do runtime nativo | `AXV_BUILD_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME` (OFF por padrão) requer ativar também adaptador Baresip, SDK verificado por hashes, caminho de `re.h` e biblioteca libre/re com hashes explícitos e exige linkagem em executável de prova de compilação. |
| 0.86 | Factory de lifecycle Baresip | `make_experimental_sdk_registration_host` compõe hooks existentes de core e UA privados dentro da thread `re_main`, condicionados a opt-in explícito, perfil LAB isolado e atestações. Não existe vínculo com a GUI. |
| 0.87 | Projeção nativa sem status falsificado | `NativeRegistrationSafetyProjection` transforma snapshot da thread proprietária em estado não sensível, sem promover callback a registro autenticado nem habilitar chamadas/mídia. |
| 0.88 | Auditoria SIP na interface Windows | Menu MAIS > Pre-ativacao SIP segura ou F3 inspeciona sintaxe sem divulgar valores, TLS selecionado, arquivo Baresip local, módulos detectados e inventário de áudio. Exibe requisitos de autenticação/TLS/SRTP que continuam pendentes. |

## Build experimental: exclusivamente laboratório

Com o SDK Baresip e o libre previamente auditados, o desenvolvedor pode optar por gerar o alvo `axv_native_registration_lab`. O CI normal e `AXEVoice.exe` **não recebem** esse alvo. Não há default que habilite produção.

São exigidos `AXV_BARESIP_INCLUDE_DIR`, `AXV_BARESIP_LIBRARY`, `AXV_BARESIP_HEADER_SHA256`, `AXV_BARESIP_LIBRARY_SHA256`, `AXV_LIBRE_INCLUDE_DIR`, `AXV_LIBRE_LIBRARY`, `AXV_LIBRE_HEADER_SHA256` e `AXV_LIBRE_LIBRARY_SHA256`. O target de prova de linkagem é somente compilado; não é executado nem inscrito no CTest.

### Pendências reais

1. Revisão de ABI, licença e proveniência dos builds Baresip/libre (incluindo hashes e todas as dependências transitivas).
2. Revisão do fluxo de credenciais e desafio-resposta no SDK, TLS com certificado validado e SRTP autenticado.
3. Exposição de eventos nativos verificados à GUI **somente após** prova independente de origem/autenticação e garantias do ciclo de vida.
4. Registro contra PABX real, chamadas nos dois sentidos, áudio PC/headset e reconexões em Windows 10/11.
5. Build e testes de unidade, integração, caixa-preta, segurança e homologação real antes de alterar `main` ou declarar GO.

**Limite de status:** não existe `VerifiedNativeApi` concedido pela factory, pelo runtime de laboratório ou pela auditoria F3. A opção Chamar continua bloqueada. Nenhuma etapa concluída em código substitui uma validação técnica ou liberação comercial.
