# Candidate0.65.2 — limpeza de credenciais na preparação do registro

## Problema e alteração

O manipulador da GUI lia a senha antes de validar a conta e preparar strings. Falhas de alocação nessas operações escapavam do manipulador e pulavam a limpeza. A avaliação de argumentos também podia criar uma cópia da senha antes de outro temporário falhar, sem chegar ao guard do motor.

A rotina usada pela GUI foi extraída para `src/native_registration_ui.h`. Conta, CA e rota agora ficam preparadas antes da leitura da senha. Guards de escopo limpam os buffers ativos UTF16/UTF8 em qualquer saída. A chamada ao motor usa referências para os argumentos já preparados e uma cópia da senha para o parâmetro protegido do motor; não cria um string SSO movido cujo tamanho ativo passa a zero. Exceções retornam um resultado fixo, sem dados da conta, em vez de atravessar WindowProc.

Senha vazia ou conversão UTF8 recusada não solicita registro. A conversão efetiva é verificada além do cálculo do tamanho. Formulário normalmente inválido não lê nem apaga a senha; uma exceção solicita a limpeza do campo. Os limites de entrada e o timeout de15000ms permanecem iguais.

## Verificação

O teste comportamental compila a rotina de produção com doubles Win32 e um motor de contrato equivalente quanto à propriedade da senha. A extração inicial manteve a ordem anterior e falhou em execução: exceções escaparam e alocações com credencial sintética foram liberadas sem limpeza. Após a correção, a matriz exige limpeza UTF16 observada, alocações UTF8 sem segredo ao liberar, resultados de recusa/erro, nenhuma solicitação de registro quando a preparação falha e tentativas normais aceitas.

Falhas das duas etapas de WideCharToMultiByte são testadas. Um segundo teste usa controles Win32 reais com senha sintética multibyte, rejeição de surrogate isolado e preservação do campo em formulário inválido. O double não pretende homologar conversão Unicode ou GUI física. Gates completos Windows/SDK/instalador devem corresponder ao commit final do PR.

## Limites e próximos itens

A garantia cobre os buffers ativos controlados por esta rotina e o contrato já testado do parâmetro do motor. Não comprova apagamento universal de cópias no Windows, heap histórico, capacidade não ativa ou otimizações do compilador. Nenhuma exposição rotineira ou exploração foi demonstrada.

As quatro auditorias desta rodada também registraram [o acompanhamento da atualização do instalador](INSTALLER-UPGRADE-FOLLOWUP.md). Remoção de arquivos antigos entre versões e atualização com app aberto continuam pendentes; a reinstalação atual usa o mesmo conteúdo. Não foram feitas exclusões genéricas de DLLs ou de dados do usuário.

Homologação Dígitro/headset, Windows físico e distribuição assinada continuam pendentes. TLS de SIP não cifra RTP; SRTP não está disponível.
