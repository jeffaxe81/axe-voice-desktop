# AXE Voice — entrega de software sem homologação de telefonia real

Decisão do titular (08/11/2026): **desconsiderar o teste com a central real para fechar a entrega de engenharia de software**. A homologação externa não foi feita e permanece **explicitamente NÃO APROVADA**. Esta decisão não é autorização para produção.

## Dois aceites independentes

**Aceite de engenharia (software candidato):** exige a evidência de oito gates no mesmo commit ou pacote: compilação/CTest Windows, integridade do upstream, ABI/reactor SDK Baresip, 1.000 casos SIP em loopback, teste de instalação/desinstalação do pacote nativo, interface Win32 compacta, candidato WinUI 3 de laboratório e controles de segurança/credenciais. A ausência de teste com PABX real **não altera** este resultado. Não marcar um gate como concluído apenas por existir código.

**Aceite de produção (bloqueado):** requer, além do aceite de engenharia, registro em central autorizada, chamadas nos dois sentidos, áudio RTP bidirecional/headset no Windows real, compatibilidade TLS/SRTP/codecs/NAT conforme destino, análise de licenças e assinatura/distribuição. Sem essas evidências o resultado é NO-GO para produção, independentemente do percentual de engenharia.

Fonte operacional de verdade no código: `src/release_scope_policy.h`, com 256 combinações de gates de engenharia × 64 combinações de gates externos no teste `tests/test_release_scope_policy.cpp`. Contrato legível por máquina: `config/release-scope.json`. O diagnóstico F2 registra expressamente a diferença sem exibir dados da conta.

## Certificado automático por commit (gate de release)

O workflow `.github/workflows/engineering-release-gate.yml` executa quando o
pipeline **AXE Voice native SDK smoke** encerra na `main`. Ele consulta somente
metadados públicos/permitidos do GitHub Actions, sem usar credenciais SIP nem
iniciar qualquer ligação, e confere que **quatro workflows** tiveram sucesso
no **mesmo SHA**: Windows CI, integridade, WinUI 3 e SDK Baresip nativo.

O certificado exige os oito gates listados em `config/release-scope.json`.
Além de resultados `success`, verifica as etapas nativas de ABI/reactor, os
1.000 registros SIP de loopback, testes locais de segurança, instalação/
desinstalação e a presença de artefatos não expirados e não vazios dos
instaladores Win32 e WinUI 3. Um build de outro commit, um pacote expirado
ou uma etapa pulada **bloqueia** o certificado.

A saída `AXE-Voice-Engineering-Acceptance-<SHA>` contém
`engineering-release-exact-sha.json`, com status `approved` ou `blocked`,
resultados de cada gate e links de evidências. Seu campo
`production_approved` permanece **false** mesmo com 8/8 gates aprovados.
Os testes da lógica estão em `tests/test_engineering_release_evidence.py`;
o validador é `scripts/verify_engineering_release.py`.

Os workflows nativo e WinUI 3 foram configurados para executar em **todos os
pushes para `main`**, evitando aceitar um pacote gerado apenas em commit
anterior. Em PRs, os filtros de arquivos permanecem para reduzir gasto de CI.
**Não tratar 8/8 gates como teste visual presencial ou homologação física.**

## O que continua fora desta entrega

- Nenhum teste automático de REGISTER, INVITE, RTP ou DTMF contra a conta/PABX de produção.
- Nada de supor sucesso em 401/407/200, áudio real ou aparelho físico com base em mocks.
- Assinatura de código e revisão de licenciamento comercial não são dispensadas para distribuição de produção.
- Ideias futuras #234–#238 não são parte desta versão.

## Entregáveis de software

- `AXE-Voice-Native-Candidate-Setup`: instalador Win32 com Baresip no perfil nativo TLS/media.
- `AXE-Voice-WinUI3-Native-Lab-Setup`, quando produzido pelo workflow: instalador separado da edição WinUI 3 de laboratório, sem aprovação de telefonia comercial.
- `AXE-Voice-Native-SIP-1000-Results`: evidência de registro SIP sintético **local**, não tráfego na central Dígitro.
- Relatórios CTest, qualidade, proveniência, integridade e matriz de aceitação com SHA.

**Importante:** a disponibilidade dos artefatos depende dos pipelines da revisão exata. O término do desenvolvimento deve ser reportado com base em evidências, não marcado automaticamente em 100%.
