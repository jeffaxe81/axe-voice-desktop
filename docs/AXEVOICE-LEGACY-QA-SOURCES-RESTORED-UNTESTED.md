# AXE Voice LAB — preparação de regressões históricas (não executadas)

Data: 25/10/2026. Base de desenvolvimento: `feature/axevoice-v073-sip-activation-gate`.

## Fontes recuperadas

| Origem | Arquivo | Cenários destinados à homologação |
| --- | --- | --- |
| PR #39 | `tests/test_sip_batch_bounds.cpp` | Lotes SIP acima da capacidade de 32, `size_t` máximo, revogação permanente da geração e nova sessão |
| PR #39 | `tests/test_sip_dial_matrix.cpp` | 6.144 combinações sintéticas de processo, evidência, SIP, sessão, motor e destino; motor real da GUI mantém `UnavailableSipEngine` |
| PR #36 | `tests/test_ui_viewport_policy.cpp` | Rolagem, limites, foco e mudança da área útil |

A fonte do limite SIP ganhou uma regressão adicional para a proteção de geração monotônica da PR #125: se um lote antigo chegar depois de uma geração mais recente autenticada **no modelo sintético de teste**, a geração atual deve ser revogada, não podendo reviver até uma sessão com ID posterior.

Os três executáveis foram adicionados ao CMake/CTest existente. `sip_batch_bounds` e `sip_dial_matrix` entram na camada de integração, `ui_viewport_policy` na unidade. Não foram criados endpoints, acessos à rede ou permissões de SIP para esses testes; o cenário de evidência nativa é **um objeto de teste** que não confere autenticação a uma aplicação de produção.

## Estado

**Apenas código-fonte e configuração alterados.** Não houve CMake/MSVC/CTest, CI manual, teste Windows ou homologação com PABX nesta rodada. O projeto mantém o caminho experimental Baresip desligado por padrão e o executável com chamadas bloqueadas. GitHub Actions pode iniciar automaticamente ao publicar a branch de feature; resultados de workflows não foram verificados nesta rodada.

Os testes recuperados não substituem o QA de callback nativo, hotplug USB/Bluetooth, mídia RTP/SRTP, 10/11 Windows, arquivo de instalador ou verificação de build/ABI do SDK. A `main` não foi alterada por esta integração.
