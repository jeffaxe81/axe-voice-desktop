# AXE Voice LAB — reconciliação de PRs antigas #28 e #21 (v0.135–v0.139)

Atualizado em 24/10/2026. **Sem compilar ou executar testes nesta rodada**.

## Entregas de código

| Versão | Alteração | Origem histórica |
|---|---|---|
| v0.135 | Ao receber `BEVENT_CALL_INCOMING` ou `BEVENT_CALL_OUTGOING`, o adaptador nativo rejeita uma chamada se `call_get_ua(call) != owner_`. | [PR #28](https://github.com/jeffaxe81/axe-voice-desktop/pull/28) |
| v0.136 | Para `BEVENT_CALL_ESTABLISHED` e `BEVENT_CALL_CLOSED`, exige simultaneamente ponteiro da chamada ativa e sua propriedade pela UA selecionada, evitando eventos que aparentam pertencer a outra sessão. | PR #28 |
| v0.137 | O header de Baresip **simulado** oferece somente a representação da API `call_get_ua`; o probe de compilação do SDK auditado usa `static_assert` da assinatura `struct ua* (*)(const struct call*)`. | PR #28 e API pública Baresip |
| v0.138 | O relatório F2 reaproveita `format_readiness_export`, contendo apenas seis indicadores de prontidão, sem copiar SIP URI, usuário, credencial, domínio, endereço ou endpoint. A cópia de diagnóstico local recebe limite de 8.192 caracteres. | [PR #21](https://github.com/jeffaxe81/axe-voice-desktop/pull/21) |
| v0.139 | Metadados CMake, janela Win32 e diagnóstico local sincronizados com `0.139.0`; README atualizado para documentar as correções e seus limites. | Consolidação |

## Revisão de segurança

- Antes das correções, o adaptador filtrava o evento pela UA (`bevent_get_ua(event)`), mas não verificava a UA interna do ponteiro `call`. A combinação de evento selecionado com chamada de outra conta revoga a sessão sem habilitar uso.
- O getter `call_get_ua(const struct call*)` consta na interface pública do Baresip. **A assinatura do SDK efetivamente instalado deve ser auditada** contra os cabeçalhos e bibliotecas pinados; `static_assert` não é teste funcional.
- O relatório F2 permanece somente informativo. Qualquer `VerifiedNativeApi` criado sinteticamente em testes não vale como autenticação no PABX; `verified_media` é `false` na GUI.
- Sem dados secretos em logs ou clipboard de diagnóstico. O comando existente de cópia explícita do destino está sujeito às suas barreiras próprias de privacidade, separadas deste relatório.
- Todas as opções de laboratório SDK nativo/WinCred permanecem `OFF` por padrão e a GUI mantém `UnavailableSipEngine`; chamadas não são ativadas.

## Pendências de homologação

Compilação MSVC Windows, auditoria da versão de Baresip e sua ABI, supervisão da vida útil de `struct call*`, ordenação de callbacks com remoção de listeners, PABX real com TLS/autenticação, RTP/SRTP e seleção de headset/PC. **Nenhum teste ou compilação executado** durante esta integração.

## Rascunhos originais

As PRs #28 e #21 continuam abertas e conflitantes; apenas os trechos pertinentes foram portados à branch mais recente, não toda a sua diff. Não devem ser encerradas como integralmente mescladas.
