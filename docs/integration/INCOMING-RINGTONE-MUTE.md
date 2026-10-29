# Candidate0.67 — controle do som recebido

## Nova função

TOQUE NORMAL/TOQUE MUDO e Ctrl+Shift+S ativam ou silenciam somente o som das chamadas recebidas. O aviso visual e ATENDER/ENCERRAR mantêm suas regras. O controle não rejeita chamadas, não ativa DND no servidor, não muda o microfone nem o áudio da conversa e não substitui o teste manual de saída.

A opção fica somente na memória desta execução. Pode ser selecionada antes de registrar, permanece ao trocar de conta e volta a normal ao reabrir o aplicativo. TOQUE MUDO aparece ao lado da duração, inclusive em mini; o atalho funciona mesmo quando o botão está oculto. As ações de copiar status e F2 continuam com o mesmo contrato de indicadores, sem nova exportação desta preferência.

Desativar o modo silencioso pode retomar o toque da mesma chamada se ainda estiver pendente. Chamadas especificamente reconhecidas por ATENDER/ENCERRAR continuam silenciadas pelo mecanismo anterior. A opção global não reconhece nem consome uma chamada.

## Implementação e verificação

O quarto argumento opcional de IncomingRingtonePolicy::request retorna pedido vazio antes de copiar o endpoint quando o som está silenciado. O player existente recebe o cancelamento cooperativo, protege gerações antigas e mantém um renderizador por vez. Unmute usa a identidade atual da chamada e respeita a prévia concorrente.

Quatro frentes de testes cobrem modo padrão/retomada, player real com renderizador controlado, autorização e observação de chamadas inalteradas, além de falha de alocação e isolamento do diagnóstico. O teste do player segura um render antigo até confirmar o cancelamento, substitui chamadas/sessões e exige que resultados obsoletos não sobrescrevam o pedido atual. O smoke nativo usa controles Win32 para verificar botão, badge, mini, despacho equivalente ao atalho, geometria e restauração ao modo normal sem mudar observação SIP.

Os gates Windows/SDK/instalador devem corresponder ao commit final do PR. Testes sintéticos verificam contrato e cancelamento; não comprovam emissão nem silêncio físico do headset.

## Limites

Cancelamento é cooperativo: um driver bloqueado pode demorar, e o buffer já entregue ao dispositivo pode terminar. Não há promessa de corte instantâneo ou prazo físico de silêncio. A preferência do toque não afeta a disponibilidade nem a confirmação de registro SIP.

Aceite Dígitro/headset, acessibilidade e Windows físicos, distribuição assinada e SRTP seguem pendentes. Atualização do instalador A→B e com app aberto continua no [acompanhamento](INSTALLER-UPGRADE-FOLLOWUP.md).
