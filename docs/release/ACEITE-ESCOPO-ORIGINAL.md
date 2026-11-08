# AXE Voice — aceitação da versão atual (escopo original)

Este roteiro se aplica ao softphone AXE Voice Windows 11, com **Baresip nativo**, interface Essencial, edição Win32 principal e prévia WinUI3. **Exclui** explicitamente as ideias de backlog #234–#238 (Smart Provisioning, Reception, Connect, Quality Guard, Voice AI). Não habilitar nenhuma delas para concluir a release.

## Decisão de escopo — 08/11/2026: entrega de engenharia sem central telefônica real

Por decisão do titular, **a homologação em PABX real NÃO é mais requisito do aceite de engenharia do software**. A entrega de engenharia poderá ser considerada 100% concluída com evidências dos oito gates automatizados/locais do mesmo commit e pacote, conforme [contrato específico](ENTREGA-DE-ENGENHARIA-SEM-PBX-REAL.md), `src/release_scope_policy.h` e `config/release-scope.json`.

**A homologação externa continua não realizada e não aprovada.** Nenhuma dispensa se aplica à liberação para produção. Os requisitos abaixo são os critérios de **homologação/produção**, não os critérios para fechar o trabalho de engenharia.

## Definição de 100% para homologação e distribuição de produção

A conclusão de produção só pode ser afirmada quando TODOS os critérios abaixo tiverem evidência no mesmo commit/pacote. Um build verde não substitui interoperabilidade na central.

| Gate | Evidência exigida | Automatizável? | Situação inicial |
|---|---|---|---|
| A. Build/packaging | Windows x64 MSVC + SDK Baresip/libre/OpenSSL fixados, manifestos/hash, artefato instalável | Sim, GitHub CI nativa | Implementado no pipeline; confirmar commit final |
| B. Testes de interface | Win32 Essencial e WinUI3 (somente prévia), sem colisão/rolagem, 4 DPIs, tema claro/escuro | Parcial (smoke + imagens) | Smoke aprovado, comparativo visual pendente |
| C. Marca/distribuição | Ícone aprovado no EXE, bandeja, Menu Iniciar e atalho opcional; instalar, atualizar, desinstalar | Parcial | Asset e CI existentes; instalação Windows física pendente |
| D. Segurança | Credenciais somente no Windows Credential Manager quando opt-in; logs sem senha; isolamento e revogação de conta | Parcial | Revisão + testes e inspeção física pendentes |
| E. SIP registro | REGISTER 401/407/200, renovação, falha autenticada, recuperação após queda de rede | Exige PBX de teste | NÃO HOMOLOGADO |
| F. Chamada originada | Chamar, ringback, atender, encerrar, CANCEL/BYE corretos | Exige 2 endpoints reais | NÃO HOMOLOGADO |
| G. Chamada recebida | INVITE real, toque, atender/rejeitar/cancelar, sem eventos perdidos, sem falsa sinalização | Exige 2 endpoints reais | NÃO HOMOLOGADO |
| H. Áudio/RTP | G.711 A/µ-law, WASAPI/headset, áudio bidirecional, troca de dispositivo, mudo/espera/DTMF/transferência disponíveis | Exige 2 endpoints reais | NÃO HOMOLOGADO |
| I. Rede/segurança media | UDP 5060, DNS, NAT/Contact, TLS/CA e SRTP quando exigido pela central | Exige PBX/rede de teste | NÃO HOMOLOGADO |
| J. Entrega | Instalação limpa, atualização e rollback, assinatura/SmartScreen, licenças/BOM, aceite responsável | Exige Windows real e revisão legal | PENDENTE |

**Medição:** o percentual de engenharia depende exclusivamente dos oito gates do contrato de software, com evidência por SHA. Os gates externos desta tabela continuam necessários para produção e não devem reduzir a porcentagem de engenharia. O marco histórico de 80% em `docs/design/MARCO-80-ACEITE.md` mede **somente design**.

## Captura real do visual (Windows físico, sem envio automático)

1. Instalar a versão candidata com hash conferido. Abrir AXE Voice, fechar qualquer janela sobreposta e deixar o discador sem telefone/conta sensíveis visíveis.
2. Selecionar o tema claro, DPI de 100%, verificar layout em 355×602 pixels lógicos (a escala de trabalho disponível pode reduzir a janela).
3. No PowerShell, no repositório: `./scripts/capture-windows-design.ps1 -Edition Win32 -Scenario dialer-light-100 -AllowLocalCapture`.
4. Repetir para tema escuro e DPI 125%/150%/200%, além do Monitor e da janela Preferências. Para a prévia, usar `-Edition WinUI3` e processo WinUI 3 aberto.
5. Revisar cada captura local, **apagar/ocultar dados pessoais e credenciais** antes de compartilhar; confrontar com a referência aprovada sem confundir mockup com screenshot.
6. Guardar relatório: data, versão exata do EXE, SHA-256, Windows/DPI, resultado sem texto sobreposto, eventuais divergências, assinatura do revisor.

Capturas ficam em `%LOCALAPPDATA%\AXEVoice\design-evidence`, fora do repositório. Não há upload automático. O script não instala componentes nem captura tela em segundo plano.

## Roteiro SIP/áudio em ambiente autorizado

- Usar ramais temporários e dois endpoints controlados na central de homologação. Nunca embutir senhas ou tokens em comandos, issues, screenshots ou artefatos.
- Registrar no cliente e conferir evidência 401/407 -> 200 OK e Contact/porta; derrubar/repor rede e verificar tempo de retomada.
- Originar chamada e receber no segundo endpoint; repetir no sentido inverso; testar rejeição, cancelamento, fim por qualquer participante, renovação do registro durante chamada.
- Conferir áudio em ambos sentidos, headset selecionado, microfone mudo, espera/retomada, DTMF SIP INFO, REFER/NOTIFY, se permitidos pelo PABX.
- Caso o destino exija criptografia SRTP ou outros codecs, somente declarar suporte após implementação explícita e testes de compatibilidade; TLS na sinalização não criptografa RTP automaticamente.
- Registrar códigos SIP/status, sem IPs internos confidenciais, número real, senha ou carga bruta de SIP. Usar resultados binários aceito/reprovado por cenário e análise de correção.

## Release e limites de licenciamento

O repositório `jeffaxe81/axe-voice-desktop` contém histórico de laboratório. A issue #130 exige revisar proveniência/licenças e estratégia de distribuição comercial antes de considerar a versão pronta para produção. Este roteiro não libera distribuição comercial nem altera a licença de dependências.

A edição WinUI3 ainda não tem discagem SIP operacional na interface de prévia; **não** deve substituir a edição Win32 nativa nesta aceitação.

## Referências

- Design/ergonomia: #227, PRs #225, #228, #230, #231
- Integração SIP real: #216, #211
- Opus/G.722/SRTP (fora do mínimo atual se não exigidos pela central): #217
- Problemas intermitentes CI: #166 e #169
- Backlog futuro expressamente excluído: #234, #235, #236, #237, #238
