# Candidate0.64 — recuperação da configuração e proveniência dos arquivos

Quatro especialistas examinaram em paralelo configuração/credenciais, diagnóstico de ciclo, processo auxiliar Windows e pacote. As regressões usam somente arquivos/senhas sintéticos; as novas probes nativas não registram conta nem enviam SIP.

| Frente | Defeito e resultado |
| --- | --- |
| Configuração | Falha de alocação escapava de configure_authenticated, liberava senha de entrada sem apagar e mantinha senha pendente anterior. Um guard apaga a entrada em todas as saídas; conta/config/áudio são preparados antes do commit. Exceção invalida configuração inativa e apaga a cópia pendente. Movimentos de commit são verificados como noexcept pelo compilador. Sessão ativa recusada permanece intacta. |
| Diagnóstico | sdk_error/sdk_stage sobreviviam a stop, repetição de stop e configuração nova. Parada e configuração aceita agora limpam erros antigos; a nova falha SDK ainda é apresentada. |
| Processo | A concatenação de aspas não preservava uma pasta terminada em backslash sob as regras CRT. O helper codifica aspas/backslashes, inclusive separador final. Mantém executável explícito, ausência de shell e restrições de handles/job. Teste Windows usa o filho inerte e observa o marker na pasta real. Não foi demonstrada injeção ou execução indevida. |
| Pacote | Arquivo extra em licenses, omitido do manifesto limitado a .h/.lib/.dll, podia entrar na cópia recursiva. Manifesto e gate agora abrangem todos os arquivos SDK, inclusive ocultos e licenças, exceto o próprio manifesto. Arquivo adicional após o build e licença alterada são recusados. README e licenças são preparados antes de gerar os hashes; regressão executa os trechos reais de geração/gate com pasta limpa e metadados antigos em cache. Não há alegação de autoexecução. |

A matriz de configuração testa os primeiros16 pontos de alocação tanto na configuração inicial quanto na substituição de credencial pendente, além do caminho que não encontra a injeção e deve continuar funcionando. Observa somente suas próprias alocações vivas e liberação de senha sintética; não certifica remoção de cópias do chamador, paginação ou terceiros. A função só pode garantir a senha já recebida por valor; alocações do chamador anteriores à entrada estão fora dessa garantia.

Os testes C++ permanecem ativos em Release. A citação correta de argumentos tem12 casos literais; o caminho real Windows e os probes SDK/instalador são gates do commit final da PR. Testes estáticos que espelhavam o texto da implementação foram substituídos por verificações de comportamento.

Não é pentest completo, assinatura de código nem SRTP. Um invasor com escrita sobre binários e o próprio manifesto ainda pode alterar ambos; o hash é vínculo de proveniência do build, não assinatura. Dígitro/headset e demais aceites externos seguem no [checklist](../RELEASE-CHECKLIST.md).
