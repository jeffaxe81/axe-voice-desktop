# Candidate0.66 — destinos recentes da sessão

## Novas funções

- RECENTES ou Ctrl+H mostra até10 destinos de pedidos de discagem aceitos localmente pelo motor, mais recente primeiro. Destinos idênticos ocupam uma linha e voltam ao topo quando solicitados novamente.
- Escolher um item preenche o campo original e permite revisá-lo. É necessário clicar CHAMAR; nenhuma seleção inicia chamada automaticamente. O botão continua verificando prova atual, sessão e registro, e o motor mantém suas próprias recusas.
- LIMPAR RECENTES esvazia a lista. Ela também é apagada quando a sessão muda, o motor para/falha ou perde sua prova válida. A lista é preservada durante uma chamada válida na mesma sessão.

Os destinos ficam apenas na memória desta execução/sessão, sem arquivo, registro do Windows ou inclusão em COPIAR STATUS/F2. Texto digitado é preservado até128 caracteres para que adicionar `sip:` na normalização não provoque truncamento ao reutilizar o campo. Entradas inválidas, caracteres fora da gramática ASCII nativa e pedidos recusados não entram na lista.

Esta lista não comprova chamada completada, atendimento, áudio, duração ou cobrança; não contém identificação de chamadas recebidas. O campo de discagem continua editável separadamente da lista.

## Proteções e testes

Cada aceitação cria um token novo. O menu captura os tokens, e a seleção valida a observação atual depois do fechamento do popup; um índice antigo não autoriza reutilizar dados de outra sessão. O helper de preenchimento não recebe um motor SIP e copia o texto antes de WM_SETTEXT, que pode reentrar na GUI.

A lista tem armazenamento limitado e faz todas as alocações antes de alterar a ordem ou remover itens. Falha de memória na mesma sessão preserva os registros e tokens. O texto digitado da tentativa é preparado antes da chamada ao motor; registrar recentes após uma aceitação não deixa uma conversão de argumento fora do guard.

Quatro frentes de testes verificam ordem/capacidade/contextos, privacidade e entradas, falhas de alocação e autorização independente. Um teste com EDIT real do Windows verifica preenchimento, falha de cópia, seleção obsoleta e destino no limite de128 caracteres. O smoke da GUI nativa cobre presença/posição do botão, indisponibilidade inicial e comando vazio sem discagem. Gates Windows/SDK/instalador devem corresponder ao commit final do PR.

## Limites

Limpeza remove registros lógicos e zera buffers ativos mantidos pela lista; não comprova apagamento forense de cópias da UI/SO/compilador ou capacidade não ativa. Não há exportação, persistência, histórico de recebidas ou estado final das chamadas nesta entrega.

Homologação Dígitro/headset, Windows físico, acessibilidade física, distribuição assinada e SRTP continuam pendentes. Atualização A→B do instalador e app aberto seguem no [acompanhamento](INSTALLER-UPGRADE-FOLLOWUP.md).
