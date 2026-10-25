# AXE Voice — critérios para promoção à main

Data da revisão: 24/10/2026.

## Escopo de segurança

- `main` contém a base histórica do projeto MicroSIP/PJSIP e **não** o motor nativo experimental Baresip do desenvolvimento.
- A branch de integração `feature/axevoice-v073-sip-activation-gate` alcançou a versão de código `0.139.0`, após a PR #112 de reconciliação parcial das PRs Draft #21 e #28. É uma base de **laboratório não homologada**; essa versão não foi promovida ao executável de `main`.
- A divergência foi originalmente observada em 372 commits de desenvolvimento e 221 arquivos; os valores mudam com novos commits. O histórico de Baresip não é substituto da validação de build/ABI da arquitetura na `main`.
- O alvo nativo de SDK e o WinCred LAB são **OFF por padrão** na branch experimental. O `AXEVoice.exe` não tem prova de autenticação, mídia ou chamadas SIP.
- **Não há autorização técnica para ativar produção ou discagem por um merge.** `main` é a base reservada para promoção após aceite.

## Política de promoção sem executar testes nesta rodada

Podem ser mescladas imediatamente alterações documentais e de governança que não mudem:
- fontes, ABI, flags de compilação, instaladores, manifestos, ações de GitHub ou dependências;
- credenciais, certificado/TLS, SIP, SRTP, RTP ou rotas de áudio;
- a possibilidade de iniciar ou receber chamadas.

Exigem validação antes de entrar como código executável da `main`:
- compilação Windows MSVC e pacote assinado conforme política de distribuição;
- integridade, ABI e licenciamento dos SDKs Baresip/libre e respectivos módulos;
- autenticação SIP em PABX real, 401/407/403/200, certificado TLS confiável, SRTP, codecs e mídia bidirecional;
- teste de isolamento de conta, cancelamento, timeout, reinício, falhas e recuperação;
- conformidade do controle de headsets/PC, audio de voz e campainha em dispositivos reais.

## Promoção documental recente

- PR #104: documentação de governança aprovada e mesclada na `main`, sem executáveis.
- PR #105: código v0.121–v0.125 mesclado **somente na branch de desenvolvimento**. O histórico da mudança está publicado como registro em [v0.125](development/AXEVOICE-NATIVE-CONTROL-V121-V125-UNTESTED.md).
- PR #107: evolução v0.126–v0.130 mesclada na branch de desenvolvimento com rejeição SIP terminal, sequência de callbacks e revogação de confirmação obsoleta. Veja [v0.130](development/AXEVOICE-SIP-REGISTER-EVIDENCE-V126-V130-UNTESTED.md). Código funcional **não promovido à main**.
- Integração total da branch de desenvolvimento na `main`: **pendente de testes e homologação**, não executados a pedido do usuário.

## Triagem dos rascunhos

A API do GitHub retornou **13 PRs Draft com conflitos** nas suas bases históricas. Suas implementações não foram mescladas à `main`; o acervo documental e a matriz de dependências estão em [docs/development/draft-archive](development/draft-archive/README.md). A integração do código original permanece pendente de reconciliação, sem execução de testes nesta rodada.

## Correções portadas dos drafts

- PR #110: a branch de desenvolvimento recebeu a validação de tamanho de lotes SIP e proteção das preferências de áudio após erro/reconexão, a partir de trechos das PRs históricas #39, #18 e #16.
- A documentação de v0.134 está [registrada na main](development/AXEVOICE-DRAFT-RECONCILIATION-V131-V134-UNTESTED.md). As PRs antigas permanecem abertas; isso não constitui merge integral nem homologação.
- Nenhuma execução de testes, build Windows ou SIP/PABX ocorreu nesta rodada.

- PR #112: melhoria de propriedade da chamada Baresip pela API `call_get_ua` e exportação de prontidão com indicadores booleanos, portadas seletivamente das PRs #28 e #21. [Registro v0.139](development/AXEVOICE-DRAFT-RECONCILIATION-V135-V139-UNTESTED.md). Continua sem testes e sem chamadas SIP liberadas.

## Situação de PRs antigas

PRs históricas Draft de SIP/áudio são experimentos empilhados. Não mesclar individualmente na `main` apenas para reduzir contagem: podem reaplicar alterações já contidas em branches mais recentes ou introduzir regressões. Manter referências até revisão de conteúdo, equivalência e dependência.

## Transparência

Esta publicação **não executou testes ou builds**. A integração do código Baresip deve ficar fora da `main` até homologação e GO explícito. Não atribuir percentual de conclusão do produto com base no percentual de commits ou PRs mescladas.
