# AXE Voice — formulário offline de homologação Windows

**Versão:** 1.1 · **Escopo:** validação física/local do aplicativo Windows, **sem PBX real**.

Abra [AXE-Voice-Windows-QA-Offline-v1.1.html](AXE-Voice-Windows-QA-Offline-v1.1.html) com Edge/Chrome em Windows 10 ou 11. O formulário não precisa de servidor e não transmite dados; o navegador pode permitir salvar informações na máquina local. **Uma ficha por computador, edição, versão e commit.**

## Executar

1. Preencha responsável, data, edição, versão do AXE Voice, identificador genérico do computador e commit SHA do instalador que efetivamente testou.
2. Siga os 44 casos (39 obrigatórios e 5 opcionais), marcando **Aprovado**, **Reprovado**, **Bloqueado** ou **Não executado**. Casos opcionais também permitem **Não se aplica**.
3. Em qualquer reprovação/bloqueio, registre o comportamento observado e uma referência local à captura de tela. Não inclua segredo SIP, ramal real, dados de clientes nem IP corporativo na ficha.
4. Selecione **Mostrar apenas reprovados/bloqueados** para triagem. **Pendências para GitHub (.md)** gera um arquivo *offline* com passos, observado, esperado e evidência por cenário. **Isso não abre nenhuma issue automaticamente**; revise/sanitize e só então publique cada problema no repositório.
5. Exporte o **JSON** para retomar preenchimento ou encaminhar os resultados para análise e **CSV** para planilha. Use impressão do navegador para gerar PDF. O JSON mantém o schema v1 para compatibilidade com a versão anterior.
6. Para emitir parecer de aprovação **local**, conclua todos os casos obrigatórios sem reprovação ou bloqueio e preencha os dados do revisor. O arquivo marca `localAcceptanceVerified` somente quando os critérios de formulário estão completos; **não** autoriza produção.

## Áreas

Instalação/distribuição (A), visual Essencial/Opção 1 (B), usabilidade/acessibilidade (C), áudio **local** e periféricos (D), segurança/configuração/diagnósticos (E) e WinUI 3 candidata de laboratório (F). Não registrar qualquer tentativa de SIP real no checklist.

**Limite de aceite:** testes com central telefônica, chamadas SIP reais e áudio RTP bidirecional foram adiados por decisão do titular, conforme [contrato de engenharia](../release/ENTREGA-DE-ENGENHARIA-SEM-PBX-REAL.md). Permanecem obrigatórios para produção. Os executáveis de laboratório não estão homologados para distribuição comercial.

## Manutenção / CI

O teste [test_windows_qa_form_contract.py](../../tests/test_windows_qa_form_contract.py) valida automaticamente contagem/IDs, contrato offline, exportação de reprovações, ausência de credenciais e bloqueios de produção. O teste é executado pelo workflow de integridade Python do repositório. Uma edição do HTML não deve remover os critérios originais ou transformar uma ficha aprovada em autorização SIP.


## Consolidar resultados de vários computadores

Depois de executar as fichas, abra
[AXE-Voice-Consolidador-Windows-v1.0.html](AXE-Voice-Consolidador-Windows-v1.0.html)
no Edge ou Chrome. Em **Selecionar fichas JSON**, escolha de uma vez os
arquivos exportados pelo formulário v1.1. A ferramenta trabalha totalmente
offline e em memória, sem usar localStorage ou qualquer conta SIP.

O painel apresenta, por máquina/edição/versão/commit, o progresso dos
39 testes obrigatórios, o aceite local recalculado, o total de falhas e
bloqueios, e quais casos reapareceram em várias máquinas. Use **Exportar
pendências CSV**, **Exportar resumo Markdown** ou **Exportar resumo JSON**
para compartilhar com a equipe e abrir as respectivas issues **após revisar
informações sensíveis**. O resumo não contém notas livres de cada teste.

**Proteções:** só importa JSON do schema v1 AXE Voice, limita tamanho,
quantidade de fichas e verifica os 44 IDs originais. Ignora qualquer alegação
de `localAcceptanceVerified=true` trazida pelo arquivo: o aceite é
recalculado usando resultados, identificação e revisor. Testes obrigatórios
não executados ou marcados 'não se aplica' impedem o aceite. Nenhuma ficha
representa homologação de PBX real ou aprovação para produção.

A suíte de integridade CI executa também
`tests/test_windows_qa_dashboard.py` e o script JavaScript isolado
`tests/qa_dashboard_node_test.cjs`, com casos sintéticos, sem discagem SIP.

