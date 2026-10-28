# AXE Voice — situação após integração na main

Atualizado em25/10/2026.

## Estado atual

Os sete PRs abertos #12, #14, #15, #23, #31, #36 e #39 foram integrados à main pelo merge `3eb43ba6972c37a64a501dbf473f208d78738b78`. Não há PR aberto restante após essa integração. As decisões de conflitos estão em [OPEN-PRS-RESOLUTION.md](integration/OPEN-PRS-RESOLUTION.md).

A main agora inclui o bootstrap Win32/Baresip de laboratório e seus componentes experimentais isolados. O projeto CMake identifica0.64.0; a branch experimental separada v0.154 não foi incorporada integralmente por esse merge. Fontes privadas locais de outro fluxo não foram publicadas.

## Evidência do merge

- CI Windows da integração: [40159743060](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40159743060),67/67 testes aprovados (38 unitários,2 caixa-preta,27 integração).
- CI Windows após merge, no SHA exato da main: [40159853827](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40159853827), concluída com sucesso, com artefato AXE-Voice-Windows-Release.
- Integridade MicroSIP após merge: [40159853861](https://github.com/jeffaxe81/axe-voice-desktop/actions/runs/40159853861), aprovada.
- Compilação cruzada Windows completa e61/61 testes portáveis foram executados durante a reconciliação.

Esses resultados comprovam os gates executados, não cobertura integral nem funcionamento SIP/RTP com uma central real. O executável GUI mantém a fronteira de SIP indisponível; o adaptador nativo permanece isolado/opcional. Não classificar esta entrega como softphone de produção.

## Avanço da auditoria de SDK

[A auditoria inicial](integration/NATIVE-SDK-AUDIT-20261025.md) compilou os probes com cabeçalhos reais Baresip/libre4.12 e executou o reactor com libre real no Linux, inclusive ASan/UBSan. Encontrou e corrigiu ordem de includes e requisitos de plataforma dos alvos opcionais. O candidato MinGW permanece rejeitado pelo gate MSVC; SIP da GUI continua bloqueado. Isso não aprova SDK Windows, PABX ou áudio real.

## Candidato nativo com instalador

A entrega do PR #132 conecta o SDK à interface em uma build candidata opcional (`AXV_ENABLE_NATIVE_SIP=ON`). Os gates passaram no Windows:70 testes gerais, registro Digest local/renovação/erros/timeout, TLS confiável e rejeição de CA/hostname inválidos, GUI nativa e instalação/desinstalação. [Evidências e download](integration/NATIVE-RELEASE-CANDIDATE.md).

A build padrão continua SDK-free. O candidato ainda não foi homologado para chamadas na Dígitro ou áudio bidirecional/headset físico, não oferece SRTP e não é uma release de produção. Os campos de senha e CA e os comandos registrar/chamar/atender/encerrar estão conectados no candidato. Na0.52 (PR #133), as seleções de microfone/voz são aplicadas ao registrar; o modo padrão do Windows permanece explícito, e endpoints ausentes bloqueiam novas chamadas/atendimento. A rota fica fixa até parar o motor; toque permanece prévia local. [Escopo0.52](integration/NATIVE-AUDIO-ROUTING.md).

## Controles locais de chamada0.53

O candidato0.53 acrescenta mudo/retomada do microfone e espera/retomada local. Comandos explícitos são vinculados à sessão/chamada exibida; os botões leem os flags do SDK. O flag local de espera não confirma aceite SIP da central; erros são relatados sem derrubar automaticamente a chamada. [Escopo e validação](integration/NATIVE-MUTE-HOLD.md). A0.54 acrescenta pedido de transferência direta e limpa o aviso antigo de áudio; transferência com consulta continua indisponível. [Escopo0.54](integration/NATIVE-TRANSFER.md).

## Próxima entrega técnica

1. SDK e candidato GUI/instalador possuem evidência automatizada; manter os gates e registrar alterações de dependências.
2. Continuar a validação de callbacks e chamadas com o candidato; não promover a produção apenas por testes locais.
3. Configurar uma conta descartável e central identificada para registro/chamada reais; não inferir configuração Dígitro a partir de testes sintéticos.
4. Comprovar mídia bidirecional, headset e política TLS/SRTP antes de declarar telefonia operacional.

O roteiro histórico abaixo registra decisões anteriores à integração e não descreve o estado atual dos PRs ou os testes do merge.

---

## Histórico anterior à integração

# AXE Voice — critérios para promoção à main

Data da revisão: 25/10/2026.

## Escopo de segurança

- `main` contém a base histórica do projeto MicroSIP/PJSIP e **não** o motor nativo experimental Baresip do desenvolvimento.
- A branch de integração `feature/axevoice-v073-sip-activation-gate` alcançou a versão de código `0.154.0`, após a PR #120 de reconciliação do diagnóstico assíncrono da aba Ajustes da PR Draft #30. É uma base de **laboratório não homologada**; essa versão não foi promovida ao executável de `main`.
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

- PR #114: observador opcional de dispositivos padrão Windows Core Audio e eMultimedia/eCommunications, novo aviso DBT_CONFIGCHANGED, e proteção de geração/sessão nos callbacks Baresip. [Registro v0.144](development/AXEVOICE-AUDIO-WATCHER-CALLBACK-FENCE-V140-V144-UNTESTED.md). **Somente no desenvolvimento**, sem build/testes executados nem media real SIP autorizada.

- PR #116: a branch de desenvolvimento recebeu a consulta assíncrona dos endpoints Windows com descarte de geração obsoleta, agrupamento de atualizações e aplicação pela UI; porta parcial da PR histórica #30. [Registro v0.149](development/AXEVOICE-ASYNC-AUDIO-V145-V149-UNTESTED.md). **Sem build/testes**, sem habilitar SIP, e a PR #30 original permanece aberta.

- **PR #120:** o botão "Dispositivos de áudio" da aba Ajustes também usa inventário em worker separado, invalidação de geração, retomada de atualizações e entrega pela UI, sem bloquear a interface. [Nota v0.154](development/AXEVOICE-SETTINGS-AUDIO-ASYNC-V150-V154-UNTESTED.md). Código **somente no desenvolvimento** e **não testado**.
- A PR #117 foi encerrada sem merge; o checklist anônimo foi incorporado pela PR #119. As antigas #18, #21 e #28 foram igualmente encerradas sem merge integral. Em 25/10/2026, restavam **10 PRs Draft abertas**.

## Situação de PRs antigas

PRs históricas Draft de SIP/áudio são experimentos empilhados. Não mesclar individualmente na `main` apenas para reduzir contagem: podem reaplicar alterações já contidas em branches mais recentes ou introduzir regressões. Manter referências até revisão de conteúdo, equivalência e dependência.

## Transparência

Esta publicação **não executou testes ou builds**. A integração do código Baresip deve ficar fora da `main` até homologação e GO explícito. Não atribuir percentual de conclusão do produto com base no percentual de commits ou PRs mescladas.
