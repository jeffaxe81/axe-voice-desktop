# AXE Voice — critérios para promoção à main

Data da revisão: 24/10/2026.

## Escopo de segurança

- `main` contém a base histórica do projeto MicroSIP/PJSIP e **não** o motor nativo experimental Baresip do desenvolvimento.
- A branch de integração `feature/axevoice-v073-sip-activation-gate` está na versão de código `0.120.0` conforme `CMakeLists.txt` na revisão. É uma base de **laboratório não homologada**.
- A divergência constatada no GitHub neste momento é 372 commits de desenvolvimento adiante e 1 commit de divergência da `main`, com cerca de 221 arquivos tocados. Não presumir merge automático e compatibilidade de build entre arquiteturas.
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

## Situação de PRs antigas

PRs históricas Draft de SIP/áudio são experimentos empilhados. Não mesclar individualmente na `main` apenas para reduzir contagem: podem reaplicar alterações já contidas em branches mais recentes ou introduzir regressões. Manter referências até revisão de conteúdo, equivalência e dependência.

## Transparência

Esta publicação **não executou testes ou builds**. A integração do código Baresip deve ficar fora da `main` até homologação e GO explícito. Não atribuir percentual de conclusão do produto com base no percentual de commits ou PRs mescladas.
