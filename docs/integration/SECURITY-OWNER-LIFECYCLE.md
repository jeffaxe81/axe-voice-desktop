# Candidate0.65 — inicialização parcial e intenção de configuração

Quatro frentes independentes testaram limpeza do proprietário, causas de falha, validação de configuração e nomes do manifesto. As regressões permanecem ativas em Release; usam somente valores sintéticos.

| Frente | Comportamento entregue |
| --- | --- |
| Limpeza parcial | O reactor omitia o callback de parada quando o início lançava exceção após adquirir estado. Agora o callback roda uma vez no proprietário após tentativa de início, inclusive parcial. Falha anterior em libre_init/fila continua sem hooks. O callback deve tolerar estado parcialmente construído; consumidores existentes têm flags/ponteiros opcionais para isso. |
| Causa da falha | Erro de loop/limpeza podia sobrescrever o fault inicial. O primeiro fault não None é preservado até a próxima execução. Encerramento por falha mantém Failed no motor, em vez de parecer parada normal, e apresenta etapa fixa sem dados da conta. Parada explícita limpa a falha visível. |
| Rejeição da conta | Uma configuração inativa A permanecia armada após rejeitar B. Agora qualquer rejeição inativa apaga senha/identidade pendente; register_account não reutiliza A. O cliente precisa fornecer novamente os dados desejados. A GUI já usava configured&&register; o reforço também protege consumidores diretos. Sessão em execução recusada permanece intacta. |
| Nomes do manifesto | Caminhos relativos canônicos, com '/', componentes não vazios e sem . / .., raízes, drives, ADS, controles, caracteres reservados, aliases de dispositivo Windows ou pontos/espaços finais. Validar antes de Join-Path/hash; nomes Unicode e colchetes literais continuam possíveis. Não foi demonstrado desvio de arquivo instalado por entrada externa; é endurecimento de esquema. |

As probes com SDK real verificam repetição após início parcial, liberação da posse global e reconhecimento de falha do ciclo. Objetos exclusivos do teste redirecionam mod_add/ua_register/re_main: a etapa parcial falha antes da conta; a etapa de ciclo suprime REGISTER e não executa o loop de rede. Um terceiro modo mantém o proprietário em loop de teste controlado para confirmar que reconfiguração recusada não altera a sessão ativa. A aplicação distribuída e as bibliotecas SDK não contêm esses redirecionamentos.

O texto de etapa não entra em F2/COPIAR STATUS: a exportação permanece somente booleana, com política, processo e evidência atuais. Não equivale a pentest completo, assinatura de código, SRTP ou homologação física. Dígitro/headset e demais aceites seguem no [checklist](../RELEASE-CHECKLIST.md).
